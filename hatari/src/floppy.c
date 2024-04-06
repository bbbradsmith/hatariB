/*
  Hatari - floppy.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  This is where we read/write sectors to/from the disk image buffers.
  NOTE: these buffers are in memory so we only need to write routines for
  the .ST format. When the buffer is to be saved (ie eject disk) we save
  it back to the original file in the correct format (.ST or .MSA).

  There are some important notes about image accessing - as we use TOS and the
  FDC to access the disk the boot-sector MUST be valid. Sometimes this is NOT
  the case! In these situations we must guess at the disk format. Eg, some disk
  images have a a boot sector which states single-sided, but the images have
  been created as double-sided. As sides are interleaved we need to read the
  image as if it was double-sided. Also note that 'NumBytesPerSector' is ALWAYS
  512 bytes, even if the boot-sector states otherwise.
  Also note that old versions of the MAKEDISK utility do not set the correct
  boot sector structure for a real ST (and also Hatari) to read it correctly.
  (PaCifiST will, however, read/write to these images as it does not perform
  FDC access as on a real ST)
*/
const char Floppy_fileid[] = "Hatari floppy.c";

#include <sys/stat.h>
#include <assert.h>
#include <SDL_endian.h>

#include "main.h"
#include "configuration.h"
#include "file.h"
#include "floppy.h"
#include "gemdos.h"
#include "hdc.h"
#include "ncr5380.h"
#include "log.h"
#include "memorySnapShot.h"
#include "st.h"
#include "msa.h"
#include "dim.h"
#include "floppy_ipf.h"
#include "floppy_stx.h"
#include "zip.h"
#include "screen.h"
#include "str.h"
#include "video.h"
#include "fdc.h"


/* Emulation drive details, eg FileName, Inserted, Changed etc... */
EMULATION_DRIVE EmulationDrives[MAX_FLOPPYDRIVES];
/* Drive A is the default */
int nBootDrive = 0;


/* Possible disk image file extensions to scan for */
static const char * const pszDiskImageNameExts[] =
{
	".msa",
	".st",
	".dim",
	".ipf",
	".raw",
	".ctr",
	".stx",
	NULL
};


/* local functions */
static bool	Floppy_EjectBothDrives(void);
static void	Floppy_DriveTransitionSetState ( int Drive , int State );

#ifdef __LIBRETRO__
extern bool core_savestate_floppy_modify;
static bool core_prevent_eject_save = false;
#endif

/*-----------------------------------------------------------------------*/
/**
 * Initialize emulation floppy drives
 */
void Floppy_Init(void)
{
	int i;

	/* Clear drive structures */
	for (i = 0; i < MAX_FLOPPYDRIVES; i++)
	{
		/* Clear structs and if floppies available, insert them */
		memset(&EmulationDrives[i], 0, sizeof(EMULATION_DRIVE));
		if (strlen(ConfigureParams.DiskImage.szDiskFileName[i]) > 0)
			Floppy_InsertDiskIntoDrive(i);
	}
}


/*-----------------------------------------------------------------------*/
/**
 * UnInitialize drives
 */
void Floppy_UnInit(void)
{
	Floppy_EjectBothDrives();
}


/*-----------------------------------------------------------------------*/
/**
 * Called on Warm/Cold Reset
 */
void Floppy_Reset(void)
{
	int	i;

	/* Cancel any pending disk change transitions */
	for (i = 0; i < MAX_FLOPPYDRIVES; i++)
	{
		EmulationDrives[i].TransitionState1 = 0;
		EmulationDrives[i].TransitionState2 = 0;
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Save/Restore snapshot of local variables('MemorySnapShot_Store' handles type)
 */
void Floppy_MemorySnapShot_Capture(bool bSave)
{
	int i;

	/* If restoring then eject old drives first! */
	if (!bSave)
#ifdef __LIBRETRO__
	{
		// if savety save is disabled, prevent the savestate restore write to disk
		if (!core_savestate_floppy_modify) core_prevent_eject_save = true;
		Floppy_EjectBothDrives();
		core_prevent_eject_save = false;
	}
#else	
		Floppy_EjectBothDrives();
#endif

	/* Save/Restore details */
	for (i = 0; i < MAX_FLOPPYDRIVES; i++)
	{
		MemorySnapShot_Store(&EmulationDrives[i].ImageType, sizeof(EmulationDrives[i].ImageType));
		MemorySnapShot_Store(&EmulationDrives[i].bDiskInserted, sizeof(EmulationDrives[i].bDiskInserted));
		MemorySnapShot_Store(&EmulationDrives[i].nImageBytes, sizeof(EmulationDrives[i].nImageBytes));
		if (!bSave && EmulationDrives[i].bDiskInserted)
		{
			EmulationDrives[i].pBuffer = malloc(EmulationDrives[i].nImageBytes);
			if (!EmulationDrives[i].pBuffer)
				perror("Floppy_MemorySnapShot_Capture");
		}
		if (EmulationDrives[i].pBuffer)
			MemorySnapShot_Store(EmulationDrives[i].pBuffer, EmulationDrives[i].nImageBytes);
#ifndef __LIBRETRO__
		MemorySnapShot_Store(EmulationDrives[i].sFileName, sizeof(EmulationDrives[i].sFileName));
#else
		MemorySnapShot_StoreFilename(EmulationDrives[i].sFileName, sizeof(EmulationDrives[i].sFileName));
#endif
		MemorySnapShot_Store(&EmulationDrives[i].bContentsChanged,sizeof(EmulationDrives[i].bContentsChanged));
		MemorySnapShot_Store(&EmulationDrives[i].bOKToSave,sizeof(EmulationDrives[i].bOKToSave));
		MemorySnapShot_Store(&EmulationDrives[i].TransitionState1,sizeof(EmulationDrives[i].TransitionState1));
		MemorySnapShot_Store(&EmulationDrives[i].TransitionState1_VBL,sizeof(EmulationDrives[i].TransitionState1_VBL));
		MemorySnapShot_Store(&EmulationDrives[i].TransitionState2,sizeof(EmulationDrives[i].TransitionState2));
		MemorySnapShot_Store(&EmulationDrives[i].TransitionState2_VBL,sizeof(EmulationDrives[i].TransitionState2_VBL));

		/* Because Floppy_EjectBothDrives() was called above before restoring (which cleared */
		/* FDC_DRIVES[].DiskInserted that was restored just before), we must call FDC_InsertFloppy */
		/* for each restored drive with an inserted disk to set FDC_DRIVES[].DiskInserted=true */
		if ( !bSave && ( EmulationDrives[i].bDiskInserted ) )
#ifndef __LIBRETRO__
			FDC_InsertFloppy ( i );
#else
			FDC_InsertFloppyRestore ( i ); // insertion resets some state that must be restored
#endif
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Find which device to boot from (hard drive or floppy).
 */
void Floppy_GetBootDrive(void)
{
	/* Default to drive A: */
	nBootDrive = 0;

	/* Boot only from hard drive if user wants this */
	if (!ConfigureParams.HardDisk.bBootFromHardDisk)
		return;

	if (bAcsiEmuOn || bScsiEmuOn || ConfigureParams.Ide[0].bUseDevice)
	{
		nBootDrive = 2;  /* Drive C */
	}
	else if (GEMDOS_EMU_ON)
	{
		int i;
		for (i = 0; i < MAX_HARDDRIVES; i++)
		{
			if (emudrives[i])
			{
				nBootDrive = emudrives[i]->drive_number;
				break;
			}
		}
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Test if disk image is write protected. Write protection can be configured
 * in the GUI. When set to "automatic", we check the file permissions of the
 * floppy disk image to decide.
 */
bool Floppy_IsWriteProtected(int Drive)
{
	if (ConfigureParams.DiskImage.nWriteProtection == WRITEPROT_OFF)
	{
		return false;
	}
	else if (ConfigureParams.DiskImage.nWriteProtection == WRITEPROT_ON)
	{
		return true;
	}
	else
	{
#ifndef __LIBRETRO__
		struct stat FloppyStat;
		/* Check whether disk is writable */
		if (stat(EmulationDrives[Drive].sFileName, &FloppyStat) == 0
		    && (FloppyStat.st_mode & S_IWUSR))
			return false;
		else
			return true;
#else
		// the image file read-only status is not accessible to the core, assume always yes
		return false;
#endif
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Test disk image for executable boot sector.
 * The boot sector is executable if the 16 bit sum of its 256 words
 * gives the value 0x1234.
 */
static bool Floppy_IsBootSectorExecutable(int Drive)
{
	Uint8 *pDiskBuffer;
	int	sum , i;

	if (EmulationDrives[Drive].bDiskInserted)
	{
		pDiskBuffer = EmulationDrives[Drive].pBuffer;

		sum = 0;
		for ( i=0 ; i<256 ; i++ )
		{
			sum += ( ( *pDiskBuffer << 8 ) + *(pDiskBuffer+1) );
			pDiskBuffer += 2;
		}

		if ( ( sum & 0xffff ) == FLOPPY_BOOT_SECTOR_EXE_SUM )		/* 0x1234 */
			return true;
	}

	return false;         /* Not executable */
}


/*-----------------------------------------------------------------------*/
/**
 * Test disk image for valid boot-sector.
 * It has been noticed that some disks, eg blank images made by the MakeDisk
 * utility or PaCifiST emulator fill in the boot-sector with incorrect information.
 * Such images cannot be read correctly using a real ST, and also Hatari.
 * To try and prevent data loss, we check for this error and flag the drive so
 * the image will not be saved back to the file.
 */
static bool Floppy_IsBootSectorOK(int Drive)
{
	Uint8 *pDiskBuffer;

	/* Does our drive have a disk in? */
	if (EmulationDrives[Drive].bDiskInserted)
	{
		pDiskBuffer = EmulationDrives[Drive].pBuffer;

		/* Check SPC (byte 13) for !=0 value. If is '0', invalid image and Hatari
		 * won't be-able to read (nor will a real ST)! */
		if ( (pDiskBuffer[13] != 0) ||  ( Floppy_IsBootSectorExecutable ( Drive ) == true ) )
		{
			return true;      /* Disk sector is OK! */
		}
		else
		{
			Log_AlertDlg(LOG_WARN, "Disk in drive %c: maybe suffers from the Pacifist/Makedisk bug.\n"
			             "If it does not work, please repair the disk first!\n", 'A' + Drive);
		}
	}

	return false;         /* Bad sector */
}


/*-----------------------------------------------------------------------*/
/**
 * Try to create disk B filename, eg 'auto_100a' becomes 'auto_100b'
 * Return new filename if think we should try, otherwise NULL
 *
 * TODO: doesn't work with images in ZIP archives
 */
static char* Floppy_CreateDiskBFileName(const char *pSrcFileName)
{
	char *szDir, *szName, *szExt;

	/* Allocate temporary memory for strings: */
	szDir = malloc(3 * FILENAME_MAX);
	if (!szDir)
	{
		perror("Floppy_CreateDiskBFileName");
		return NULL;
	}
	szName = szDir + FILENAME_MAX;
	szExt = szName + FILENAME_MAX;

	/* So, first split name into parts */
	File_SplitPath(pSrcFileName, szDir, szName, szExt);

	/* All OK? */
	if (strlen(szName) > 0)
	{
		char *last = &(szName[strlen(szName)-1]);
		/* Now, did filename end with an 'A' or 'a' */
		if (*last == 'A' || *last == 'a')
		{
			char *szFull;
			/* Change 'A' to a 'B' */
			*last += 1;
			/* And re-build name into destination */
			szFull = File_MakePath(szDir, szName, szExt);
			if (szFull)
			{
				/* Does file exist? */
				if (File_Exists(szFull))
				{
					free(szDir);
					return szFull;
				}
				free(szFull);
			}
		}
	}
	free(szDir);
	return NULL;
}


/*-----------------------------------------------------------------------*/
/**
 * Set floppy image to be ejected
 */
const char* Floppy_SetDiskFileNameNone(int Drive)
{
	assert(Drive >= 0 && Drive < MAX_FLOPPYDRIVES);
	ConfigureParams.DiskImage.szDiskFileName[Drive][0] = '\0';
	return ConfigureParams.DiskImage.szDiskFileName[Drive];
}

/*-----------------------------------------------------------------------*/
/**
 * Set given floppy drive image file name and handle
 * different image extensions.
 * Return corrected file name on success and NULL on failure.
 */
const char* Floppy_SetDiskFileName(int Drive, const char *pszFileName, const char *pszZipPath)
{
#ifndef __LIBRETRO__
	char *filename;
	int i;

	/* setting to empty or "none" ejects */
	if (!*pszFileName || strcasecmp(pszFileName, "none") == 0)
	{
		return Floppy_SetDiskFileNameNone(Drive);
	}
	/* See if file exists, and if not, get/add correct extension */
	if (!File_Exists(pszFileName))
		filename = File_FindPossibleExtFileName(pszFileName, pszDiskImageNameExts);
	else
		filename = strdup(pszFileName);
	if (!filename)
	{
		Log_AlertDlg(LOG_INFO, "Image '%s' not found", pszFileName);
		return NULL;
	}

	/* If we insert a disk into Drive A, should we try to put disk 2 into drive B? */
	if (Drive == 0 && ConfigureParams.DiskImage.bAutoInsertDiskB)
	{
		/* Attempt to make up second filename, eg was 'auto_100a' to 'auto_100b' */
		char *szDiskBFileName = Floppy_CreateDiskBFileName(filename);
		if (szDiskBFileName)
		{
			/* recurse with Drive B */
			Floppy_SetDiskFileName(1, szDiskBFileName, pszZipPath);
			free(szDiskBFileName);
		}
	}

	/* validity checks */
	assert(Drive >= 0 && Drive < MAX_FLOPPYDRIVES);
	for (i = 0; i < MAX_FLOPPYDRIVES; i++)
	{
		if (i == Drive)
			continue;
		/* prevent inserting same image to multiple drives */
		if (strcmp(filename, ConfigureParams.DiskImage.szDiskFileName[i]) == 0)
		{
			Log_AlertDlg(LOG_ERROR, "ERROR: Cannot insert same floppy to multiple drives!");
			free(filename);
			return NULL;
		}
	}

	/* do the changes */
	if (pszZipPath)
		strcpy(ConfigureParams.DiskImage.szDiskZipPath[Drive], pszZipPath);
	else
		ConfigureParams.DiskImage.szDiskZipPath[Drive][0] = '\0';
	strlcpy(ConfigureParams.DiskImage.szDiskFileName[Drive], filename,
	        sizeof(ConfigureParams.DiskImage.szDiskFileName[Drive]));
	free(filename);
	//File_MakeAbsoluteName(ConfigureParams.DiskImage.szDiskFileName[Drive]);
#else
	(void)pszDiskImageNameExts;
	(void)Floppy_CreateDiskBFileName;
	strlcpy(ConfigureParams.DiskImage.szDiskFileName[Drive], pszFileName,
	        sizeof(ConfigureParams.DiskImage.szDiskFileName[Drive]));
	(void)pszZipPath;
#endif
	return ConfigureParams.DiskImage.szDiskFileName[Drive];
}

/*-----------------------------------------------------------------------*/
/**
 * Update the drive when a disk is inserted or ejected. Depending on the state,
 * we change the Write Protect bit for the drive (the TOS and other programs
 * monitor this bit to detect that a disk was changed in the drive ; see fdc.c)
 * The floppy drive transition can be a single action ("eject" or "insert"), or
 * two actions ("eject then insert" or "insert then eject").
 * First action is stored in State1 ; State2 store the second (or last) action.
 * In case the user eject/insert several disks before returning to emulation,
 * State1 will contain the first action, and State2 the latest action (intermediate
 * actions are ignored, as they wouldn't be seen while the emulation is paused).
 * Each action will take FLOPPY_DRIVE_TRANSITION_DELAY_VBL VBLs to execute,
 * see fdc.c for details.
 */
static void	Floppy_DriveTransitionSetState ( int Drive , int State )
{
	/* First, update State1 and State2 depending on the current VBL number */
	/* (we discard the return value as we don't want to update FDC.STR now) */
	Floppy_DriveTransitionUpdateState ( Drive );

	/* If State1 is not defined yet, we set it */
	if ( EmulationDrives[Drive].TransitionState1 == 0 )
	{
		EmulationDrives[Drive].TransitionState1 = State;
		EmulationDrives[Drive].TransitionState1_VBL = nVBLs;
		/* Cancel State2 in case we start a new transition before State2 was over */
		EmulationDrives[Drive].TransitionState2 = 0;	
	}

	/* State1 is already set, so we set State2 */
	else
	{
		/* If State2 == State1, ignore it (eg : two inserts in a row) */
		if ( EmulationDrives[Drive].TransitionState1 == State )
			EmulationDrives[Drive].TransitionState2 = 0;
		else
		{
			/* Set State2 just after State1 ends */
			EmulationDrives[Drive].TransitionState2 = State;
			EmulationDrives[Drive].TransitionState2_VBL = EmulationDrives[Drive].TransitionState1_VBL + FLOPPY_DRIVE_TRANSITION_DELAY_VBL;
		}
	}
//fprintf ( stderr , "drive transition state1 %d %d state2 %d %d\n" ,
//	  EmulationDrives[Drive].TransitionState1 , EmulationDrives[Drive].TransitionState1_VBL,
//	  EmulationDrives[Drive].TransitionState2 , EmulationDrives[Drive].TransitionState2_VBL );
}


/*-----------------------------------------------------------------------*/
/**
 * When a disk is inserted or ejected, each transition has 2 phases that
 * lasts FLOPPY_DRIVE_TRANSITION_DELAY_VBL VBLs. This function checks if
 * we're during one of these transition phases and tells if the Write
 * Protect signal should be overwritten.
 * Returns 0 if there's no change, 1 if WPRT should be forced to 1 and
 * -1 if WPRT should be forced to 0 (see fdc.c for details).
 */
int	Floppy_DriveTransitionUpdateState ( int Drive )
{
	int	Force = 0;

	if ( EmulationDrives[Drive].TransitionState1 != 0 )
	{
		if ( nVBLs >= EmulationDrives[Drive].TransitionState1_VBL + FLOPPY_DRIVE_TRANSITION_DELAY_VBL )
			EmulationDrives[Drive].TransitionState1 = 0;	/* State1's delay elapsed */
		else
		{
			if ( EmulationDrives[Drive].TransitionState1 == FLOPPY_DRIVE_TRANSITION_STATE_INSERT )
				Force = 0;				/* Insert : keep WPRT */
			else
				Force = 1;				/* Eject : set WPRT */
		}
	}

	if ( ( EmulationDrives[Drive].TransitionState2 != 0 )
	  && ( nVBLs >= EmulationDrives[Drive].TransitionState2_VBL ) )
	{
		if ( nVBLs >= EmulationDrives[Drive].TransitionState2_VBL + FLOPPY_DRIVE_TRANSITION_DELAY_VBL )
			EmulationDrives[Drive].TransitionState2 = 0;	/* State2's delay elapsed */
		else
		{
			if ( EmulationDrives[Drive].TransitionState2 == FLOPPY_DRIVE_TRANSITION_STATE_INSERT )
				Force = 0;				/* Insert : keep WPRT */
			else
				Force = 1;				/* Eject : set WPRT */
		}
	}


	return Force;
}


#ifdef __LIBRETRO__
extern bool core_floppy_insert(int drive, const char* filename, void* data, unsigned int size, void* extra_data, unsigned int extra_size);
extern void core_floppy_eject(int drive);
extern Uint8* core_floppy_file_read(const char *pszFileName, long *pFileSize, bool extra);
extern const char* core_floppy_inserted(int drive);
extern void core_floppy_changed(int drive);
static void* floppy_data[2] = {NULL,NULL};
static void* floppy_extra_data[2] = {NULL,NULL};
static unsigned int floppy_size[2] = {0,0};
static unsigned int floppy_extra_size[2] = {0,0};
static int floppy_read_drive = 0;
extern bool core_floppy_insert(int drive, const char* filename, void* data, unsigned int size, void* extra_data, unsigned int extra_size)
{
	floppy_data[drive] = data;
	floppy_size[drive] = size;
	floppy_extra_data[drive] = extra_data;
	floppy_extra_size[drive] = extra_size;
	Floppy_SetDiskFileName(drive, filename, NULL);
	return Floppy_InsertDiskIntoDrive(drive);
}
void core_floppy_eject(int drive)
{
	Floppy_EjectDiskFromDrive(drive);
	Floppy_SetDiskFileNameNone(drive);
	floppy_data[drive] = NULL;
	floppy_size[drive] = 0;
}
bool core_floppy_file_extra(void)
{
	return floppy_extra_data[floppy_read_drive] != NULL;
}
Uint8* core_floppy_file_read(const char *pszFileName, long *pFileSize, bool extra)
{
	// replaces File_Read
	// does not handle gz or zip files
	const Uint8* data = extra ? floppy_extra_data[floppy_read_drive] : floppy_data[floppy_read_drive];
	const unsigned int size = extra ? floppy_extra_size[floppy_read_drive] : floppy_size[floppy_read_drive];
	Uint8* data_out;
	(void)pszFileName;
	if (data == NULL || size == 0) return NULL;
	data_out = malloc(size);
	if (data_out == NULL) return NULL;
	memcpy(data_out,data,size);
	if (pFileSize) *pFileSize = (long)size;
	return data_out;
}
const char* core_floppy_inserted(int drive)
{
	if (!EmulationDrives[drive].bDiskInserted) return NULL;
	return ConfigureParams.DiskImage.szDiskFileName[drive];
}
void core_floppy_changed(int drive)
{
	EmulationDrives[drive].bContentsChanged = true;
}
#endif

/*-----------------------------------------------------------------------*/
/**
 * Insert previously set disk file image into floppy drive.
 * The WHOLE image is copied into Hatari drive buffers, and
 * uncompressed if necessary.
 * Return TRUE on success, false otherwise.
 */
bool Floppy_InsertDiskIntoDrive(int Drive)
{
	long	nImageBytes = 0;
	char	*filename;
	int	ImageType = FLOPPY_IMAGE_TYPE_NONE;

	/* Eject disk, if one is inserted (doesn't inform user) */
	assert(Drive >= 0 && Drive < MAX_FLOPPYDRIVES);
	Floppy_EjectDiskFromDrive(Drive);

	filename = ConfigureParams.DiskImage.szDiskFileName[Drive];
	if (!filename[0])
	{
		return true; /* only do eject */
	}
#ifndef __LIBRETRO__
	if (!File_Exists(filename))
	{
		Log_AlertDlg(LOG_INFO, "Image '%s' not found", filename);
		return false;
	}
#else
	floppy_read_drive = Drive;
#endif

	/* Check disk image type and read the file: */
	if (MSA_FileNameIsMSA(filename, true))
		EmulationDrives[Drive].pBuffer = MSA_ReadDisk(Drive, filename, &nImageBytes, &ImageType);
	else if (ST_FileNameIsST(filename, true))
		EmulationDrives[Drive].pBuffer = ST_ReadDisk(Drive, filename, &nImageBytes, &ImageType);
	else if (DIM_FileNameIsDIM(filename, true))
		EmulationDrives[Drive].pBuffer = DIM_ReadDisk(Drive, filename, &nImageBytes, &ImageType);
	else if (IPF_FileNameIsIPF(filename, true))
		EmulationDrives[Drive].pBuffer = IPF_ReadDisk(Drive, filename, &nImageBytes, &ImageType);
	else if (STX_FileNameIsSTX(filename, true))
		EmulationDrives[Drive].pBuffer = STX_ReadDisk(Drive, filename, &nImageBytes, &ImageType);
#ifndef __LIBRETRO__
	else if (ZIP_FileNameIsZIP(filename))
	{
		const char *zippath = ConfigureParams.DiskImage.szDiskZipPath[Drive];
		EmulationDrives[Drive].pBuffer = ZIP_ReadDisk(Drive, filename, zippath, &nImageBytes, &ImageType);
	}
#endif

	if ( (EmulationDrives[Drive].pBuffer == NULL) || ( ImageType == FLOPPY_IMAGE_TYPE_NONE ) )
	{
		Log_AlertDlg(LOG_INFO, "Image '%s' filename extension, or content unrecognized", filename);
		return false;
	}

	/* For IPF, call specific function to handle the inserted image */
	if ( ImageType == FLOPPY_IMAGE_TYPE_IPF )
	{
		if ( IPF_Insert ( Drive , EmulationDrives[Drive].pBuffer , nImageBytes ) == false )
		{
			free ( EmulationDrives[Drive].pBuffer );
			EmulationDrives[Drive].pBuffer = NULL;
			Log_AlertDlg(LOG_INFO, "IPF image '%s' loading failed", filename);
			return false;
		}
	}

	/* For STX, call specific function to handle the inserted image */
	else if ( ImageType == FLOPPY_IMAGE_TYPE_STX )
	{
		if ( STX_Insert ( Drive , filename , EmulationDrives[Drive].pBuffer , nImageBytes ) == false )
		{
			free ( EmulationDrives[Drive].pBuffer );
			EmulationDrives[Drive].pBuffer = NULL;
			Log_AlertDlg(LOG_INFO, "STX image '%s' loading failed", filename);
			return false;
		}
	}

	/* Store image filename (required for ejecting the disk later!) */
	strcpy(EmulationDrives[Drive].sFileName, filename);

	/* Store size and set drive states */
	EmulationDrives[Drive].ImageType = ImageType;
	EmulationDrives[Drive].nImageBytes = nImageBytes;
	EmulationDrives[Drive].bDiskInserted = true;
	EmulationDrives[Drive].bContentsChanged = false;

	if ( ( ImageType == FLOPPY_IMAGE_TYPE_ST ) || ( ImageType == FLOPPY_IMAGE_TYPE_MSA )
	  || ( ImageType == FLOPPY_IMAGE_TYPE_DIM ) )
		EmulationDrives[Drive].bOKToSave = Floppy_IsBootSectorOK(Drive);
	else if ( ImageType == FLOPPY_IMAGE_TYPE_STX )
		EmulationDrives[Drive].bOKToSave = true;
	else if ( ImageType == FLOPPY_IMAGE_TYPE_IPF )
		EmulationDrives[Drive].bOKToSave = false;
	else
		EmulationDrives[Drive].bOKToSave = false;

	Floppy_DriveTransitionSetState ( Drive , FLOPPY_DRIVE_TRANSITION_STATE_INSERT );
	FDC_InsertFloppy ( Drive );

	Log_Printf(LOG_INFO, "Inserted disk '%s' to drive %c:.",
		   filename, 'A'+Drive);
	return true;
}


/*-----------------------------------------------------------------------*/
/**
 * Eject disk from floppy drive, save contents back to PCs hard-drive if
 * they have been changed.
 * Return true if there was something to eject.
 */
bool Floppy_EjectDiskFromDrive(int Drive)
{
	bool bEjected = false;

	/* Does our drive have a disk in? */
	if (EmulationDrives[Drive].bDiskInserted)
	{
		bool bSaved = false;
		char *psFileName = EmulationDrives[Drive].sFileName;

		/* OK, has contents changed? If so, need to save */
#ifndef __LIBRETRO__
		if (EmulationDrives[Drive].bContentsChanged)
#else
		// LIBRETRO can block saves during restore to prevent high disk activity
		if (EmulationDrives[Drive].bContentsChanged && !core_prevent_eject_save)
#endif
		{
			/* Is OK to save image (if boot-sector is bad, don't allow a save) */
			if (EmulationDrives[Drive].bOKToSave)
			{
				/* Save as .MSA, .ST, .DIM, .IPF or .STX image? */
				if (MSA_FileNameIsMSA(psFileName, true))
					bSaved = MSA_WriteDisk(Drive, psFileName, EmulationDrives[Drive].pBuffer, EmulationDrives[Drive].nImageBytes);
				else if (ST_FileNameIsST(psFileName, true))
					bSaved = ST_WriteDisk(Drive, psFileName, EmulationDrives[Drive].pBuffer, EmulationDrives[Drive].nImageBytes);
				else if (DIM_FileNameIsDIM(psFileName, true))
					bSaved = DIM_WriteDisk(Drive, psFileName, EmulationDrives[Drive].pBuffer, EmulationDrives[Drive].nImageBytes);
				else if (IPF_FileNameIsIPF(psFileName, true))
					bSaved = IPF_WriteDisk(Drive, psFileName, EmulationDrives[Drive].pBuffer, EmulationDrives[Drive].nImageBytes);
				else if (STX_FileNameIsSTX(psFileName, true))
					bSaved = STX_WriteDisk(Drive, psFileName, EmulationDrives[Drive].pBuffer, EmulationDrives[Drive].nImageBytes);
#ifndef __LIBRETRO__
				else if (ZIP_FileNameIsZIP(psFileName))
					bSaved = ZIP_WriteDisk(Drive, psFileName, EmulationDrives[Drive].pBuffer, EmulationDrives[Drive].nImageBytes);
#endif
				if (bSaved)
					Log_Printf(LOG_INFO, "Updated the contents of floppy image '%s'.", psFileName);
				else
					Log_Printf(LOG_INFO, "Writing of this format failed or not supported, discarded the contents\n of floppy image '%s'.", psFileName);
			} else
				Log_Printf(LOG_INFO, "Writing not possible, discarded the contents of floppy image\n '%s'.", psFileName);
		}

		/* Inform user that disk has been ejected! */
		Log_Printf(LOG_INFO, "Floppy %c: has been removed from drive.",
			   'A'+Drive);

		Floppy_DriveTransitionSetState ( Drive , FLOPPY_DRIVE_TRANSITION_STATE_EJECT );
		FDC_EjectFloppy ( Drive );
		bEjected = true;
	}

	/* Free data used by this IPF image */
	if ( EmulationDrives[Drive].ImageType == FLOPPY_IMAGE_TYPE_IPF )
		IPF_Eject ( Drive );
	/* Free data used by this STX image */
	else if ( EmulationDrives[Drive].ImageType == FLOPPY_IMAGE_TYPE_STX )
		STX_Eject ( Drive );


	/* Drive is now empty */
	if (EmulationDrives[Drive].pBuffer != NULL)
	{
		free(EmulationDrives[Drive].pBuffer);
		EmulationDrives[Drive].pBuffer = NULL;
	}

	EmulationDrives[Drive].sFileName[0] = '\0';
	EmulationDrives[Drive].ImageType = FLOPPY_IMAGE_TYPE_NONE;
	EmulationDrives[Drive].nImageBytes = 0;
	EmulationDrives[Drive].bDiskInserted = false;
#ifndef __LIBRETRO__
	if (!core_prevent_eject_save)
#endif
	EmulationDrives[Drive].bContentsChanged = false;
	EmulationDrives[Drive].bOKToSave = false;

	return bEjected;
}


/*-----------------------------------------------------------------------*/
/**
 * Eject all disk image from floppy drives - call when quit.
 * Return true if there was something to eject.
 */
static bool Floppy_EjectBothDrives(void)
{
	bool bEjectedA, bEjectedB;

	/* Eject disk images from drives 'A' and 'B' */
	bEjectedA = Floppy_EjectDiskFromDrive(0);
	bEjectedB = Floppy_EjectDiskFromDrive(1);

	return bEjectedA || bEjectedB;
}


/*-----------------------------------------------------------------------*/
/**
 * Double-check information read from boot-sector as this is sometimes found to
 * be incorrect. The .ST image file should be divisible by the sector size,
 * the sectors per track. the number of tracks and the number of sides.
 * NOTE - Pass information from boot-sector to this function (if we can't
 * decide we leave it alone).
 */
static void Floppy_DoubleCheckFormat(long nDiskSize, long nSectorsPerDisk, Uint16 *pnSides, Uint16 *pnSectorsPerTrack)
{
	long	TotalSectors;
	int	Sides_fixed;
	int	SectorsPerTrack_fixed;

	/* Now guess at number of sides */
	if ( nDiskSize < (500*1024) )				/* If size >500k assume 2 sides */
		Sides_fixed = 1;
	else
		Sides_fixed = 2;

	/* Number of 512 bytes sectors for this disk image */
	TotalSectors = nDiskSize / 512;

	/* Check some common values */
	if      ( TotalSectors == 80*9*Sides_fixed )	{ SectorsPerTrack_fixed = 9; }
	else if ( TotalSectors == 81*9*Sides_fixed )	{ SectorsPerTrack_fixed = 9; }
	else if ( TotalSectors == 82*9*Sides_fixed )	{ SectorsPerTrack_fixed = 9; }
	else if ( TotalSectors == 83*9*Sides_fixed )	{ SectorsPerTrack_fixed = 9; }
	else if ( TotalSectors == 84*9*Sides_fixed )	{ SectorsPerTrack_fixed = 9; }
	else if ( TotalSectors == 80*10*Sides_fixed )	{ SectorsPerTrack_fixed = 10; }
	else if ( TotalSectors == 81*10*Sides_fixed )	{ SectorsPerTrack_fixed = 10; }
	else if ( TotalSectors == 82*10*Sides_fixed )	{ SectorsPerTrack_fixed = 10; }
	else if ( TotalSectors == 83*10*Sides_fixed )	{ SectorsPerTrack_fixed = 10; }
	else if ( TotalSectors == 84*10*Sides_fixed )	{ SectorsPerTrack_fixed = 10; }
	else if ( TotalSectors == 80*11*Sides_fixed )	{ SectorsPerTrack_fixed = 11; }
	else if ( TotalSectors == 81*11*Sides_fixed )	{ SectorsPerTrack_fixed = 11; }
	else if ( TotalSectors == 82*11*Sides_fixed )	{ SectorsPerTrack_fixed = 11; }
	else if ( TotalSectors == 83*11*Sides_fixed )	{ SectorsPerTrack_fixed = 11; }
	else if ( TotalSectors == 84*11*Sides_fixed )	{ SectorsPerTrack_fixed = 11; }
	else if ( TotalSectors == 80*12*Sides_fixed )	{ SectorsPerTrack_fixed = 12; }
	else if ( TotalSectors == 81*12*Sides_fixed )	{ SectorsPerTrack_fixed = 12; }
	else if ( TotalSectors == 82*12*Sides_fixed )	{ SectorsPerTrack_fixed = 12; }
	else if ( TotalSectors == 83*12*Sides_fixed )	{ SectorsPerTrack_fixed = 12; }
	else if ( TotalSectors == 84*12*Sides_fixed )	{ SectorsPerTrack_fixed = 12; }

	/* unknown combination, assume boot sector is correct */
	else						{ SectorsPerTrack_fixed = *pnSectorsPerTrack; }

	/* Valid new values if necessary */
	if ( ( *pnSides != Sides_fixed ) || ( *pnSectorsPerTrack != SectorsPerTrack_fixed ) )
	{
#if 0
		int TracksPerDisk_fixed = TotalSectors / ( SectorsPerTrack_fixed * Sides_fixed );
		Log_Printf(LOG_WARN, "Floppy_DoubleCheckFormat: boot sector doesn't match disk image's size :"
			" total sectors %ld->%ld sides %d->%d sectors %d->%d tracks %d\n",
			nSectorsPerDisk , TotalSectors , *pnSides , Sides_fixed , *pnSectorsPerTrack , SectorsPerTrack_fixed , TracksPerDisk_fixed );
#endif
		*pnSides = Sides_fixed;
		*pnSectorsPerTrack = SectorsPerTrack_fixed;
	}
}


/*-----------------------------------------------------------------------*/
/**
 * Find details of disk image. We need to do this via a function as sometimes the boot-block
 * is not actually correct with the image - some demos/game disks have incorrect bytes in the
 * boot sector and this attempts to find the correct values.
 */
void Floppy_FindDiskDetails(const Uint8 *pBuffer, int nImageBytes,
                            Uint16 *pnSectorsPerTrack, Uint16 *pnSides)
{
	Uint16 nSectorsPerTrack, nSides, nSectorsPerDisk;

	/* First do check to find number of sectors and bytes per sector */
	nSectorsPerTrack = SDL_SwapLE16(*(const Uint16 *)(pBuffer+24));   /* SPT */
	nSides = SDL_SwapLE16(*(const Uint16 *)(pBuffer+26));             /* SIDE */
	nSectorsPerDisk = pBuffer[19] | (pBuffer[20] << 8);               /* total sectors */

	/* If the number of sectors announced is incorrect, the boot-sector may
	 * contain incorrect information, eg the 'Eat.st' demo, or wrongly imaged
	 * single/double sided floppies... */
	if (nSectorsPerDisk != nImageBytes/512)
		Floppy_DoubleCheckFormat(nImageBytes, nSectorsPerDisk, &nSides, &nSectorsPerTrack);

	/* And set values */
	if (pnSectorsPerTrack)
		*pnSectorsPerTrack = nSectorsPerTrack;
	if (pnSides)
		*pnSides = nSides;
}


/*-----------------------------------------------------------------------*/
/**
 * Read sectors from floppy disk image, return TRUE if all OK
 * NOTE Pass -ve as Count to read whole track
 */
bool Floppy_ReadSectors(int Drive, Uint8 **pBuffer, Uint16 Sector,
                        Uint16 Track, Uint16 Side, short Count,
                        int *pnSectorsPerTrack, int *pSectorSize)
{
	Uint8 *pDiskBuffer;
	Uint16 nSectorsPerTrack, nSides, nBytesPerTrack;
	long Offset;
	int nImageTracks;

	/* Do we have a disk in our drive? */
	if (EmulationDrives[Drive].bDiskInserted)
	{
		/* Looks good */
		pDiskBuffer = EmulationDrives[Drive].pBuffer;

		/* Find #sides and #sectors per track */
		Floppy_FindDiskDetails(EmulationDrives[Drive].pBuffer,EmulationDrives[Drive].nImageBytes,&nSectorsPerTrack,&nSides);
		nImageTracks = ((EmulationDrives[Drive].nImageBytes / NUMBYTESPERSECTOR) / nSectorsPerTrack) / nSides;

		/* Need to read whole track? */
		if (Count<0)
			Count = nSectorsPerTrack;
		/* Write back number of sector per track */
		if (pnSectorsPerTrack)
			*pnSectorsPerTrack = nSectorsPerTrack;

		if (pSectorSize)
			*pSectorSize = NUMBYTESPERSECTOR;			/* Size is 512 bytes for ST/MSA */

		/* Debug check as if we read over the end of a track we read into side 2! */
		if (Count > nSectorsPerTrack)
		{
			Log_Printf(LOG_DEBUG, "Floppy_ReadSectors: reading over single track\n");
		}

		/* Check that the side number (0 or 1) does not exceed the amount of sides (1 or 2).
		 * (E.g. some games like Drakkhen or Bolo can load additional data from the
		 * second disk side, but they also work with single side floppy drives) */
		if (Side >= nSides)
		{
			Log_Printf(LOG_DEBUG, "Floppy_ReadSectors: Program tries to read from side %i "
			           "of a disk image with %i sides!\n", Side+1, nSides);
			return false;
		}

		/* Check if track number is in range */
		if (Track >= nImageTracks)
		{
			Log_Printf(LOG_DEBUG, "Floppy_ReadSectors: Program tries to read from track %i "
			           "of a disk image with only %i tracks!\n", Track, nImageTracks);
			return false;
		}

		/* Check if sector number is in range */
		if (Sector <= 0 || Sector > nSectorsPerTrack)
		{
			Log_Printf(LOG_DEBUG, "Floppy_ReadSectors: Program tries to read from sector %i "
			           "of a disk image with %i sectors per track!\n", Sector, nSectorsPerTrack);
			return false;
		}

		/* Seek to sector */
		nBytesPerTrack = NUMBYTESPERSECTOR*nSectorsPerTrack;
		Offset = nBytesPerTrack*Side;                 /* First seek to side */
		Offset += (nBytesPerTrack*nSides)*Track;      /* Then seek to track */
		Offset += (NUMBYTESPERSECTOR*(Sector-1));     /* And finally to sector */

		/* Return a pointer to the sectors data (usually 512 bytes per sector) */
		*pBuffer = pDiskBuffer+Offset;

		return true;
	}

	return false;
}


/*-----------------------------------------------------------------------*/
/**
 * Write sectors from floppy disk image, return TRUE if all OK
 * NOTE Pass -ve as Count to write whole track
 */
bool Floppy_WriteSectors(int Drive, Uint8 *pBuffer, Uint16 Sector,
                         Uint16 Track, Uint16 Side, short Count,
                         int *pnSectorsPerTrack, int *pSectorSize)
{
	Uint8 *pDiskBuffer;
	Uint16 nSectorsPerTrack, nSides, nBytesPerTrack;
	long Offset;
	int nImageTracks;

	/* Do we have a writable disk in our drive? */
	if (EmulationDrives[Drive].bDiskInserted && !Floppy_IsWriteProtected(Drive))
	{
		/* Looks good */
		pDiskBuffer = EmulationDrives[Drive].pBuffer;

		/* Find #sides and #sectors per track */
		Floppy_FindDiskDetails(EmulationDrives[Drive].pBuffer,EmulationDrives[Drive].nImageBytes,&nSectorsPerTrack,&nSides);
		nImageTracks = ((EmulationDrives[Drive].nImageBytes / NUMBYTESPERSECTOR) / nSectorsPerTrack) / nSides;

		/* Need to write whole track? */
		if (Count<0)
			Count = nSectorsPerTrack;
		/* Write back number of sector per track */
		if (pnSectorsPerTrack)
			*pnSectorsPerTrack = nSectorsPerTrack;

		if (pSectorSize)
			*pSectorSize = NUMBYTESPERSECTOR;			/* Size is 512 bytes for ST/MSA */

		/* Debug check as if we write over the end of a track we write into side 2! */
		if (Count > nSectorsPerTrack)
		{
			Log_Printf(LOG_DEBUG, "Floppy_WriteSectors: writing over single track\n");
		}

		/* Check that the side number (0 or 1) does not exceed the amount of sides (1 or 2). */
		if (Side >= nSides)
		{
			Log_Printf(LOG_DEBUG, "Floppy_WriteSectors: Program tries to write to side %i "
			           "of a disk image with %i sides!\n", Side+1, nSides);
			return false;
		}

		/* Check if track number is in range */
		if (Track >= nImageTracks)
		{
			Log_Printf(LOG_DEBUG, "Floppy_WriteSectors: Program tries to write to track %i "
			           "of a disk image with only %i tracks!\n", Track, nImageTracks);
			return false;
		}

		/* Check if sector number is in range */
		if (Sector <= 0 || Sector > nSectorsPerTrack)
		{
			Log_Printf(LOG_DEBUG, "Floppy_WriteSectors: Program tries to write to sector %i "
			           "of a disk image with %i sectors per track!\n", Sector, nSectorsPerTrack);
			return false;
		}

		/* Seek to sector */
		nBytesPerTrack = NUMBYTESPERSECTOR*nSectorsPerTrack;
		Offset = nBytesPerTrack*Side;               /* First seek to side */
		Offset += (nBytesPerTrack*nSides)*Track;    /* Then seek to track */
		Offset += (NUMBYTESPERSECTOR*(Sector-1));   /* And finally to sector */

		/* Write sectors (usually 512 bytes per sector) */
		memcpy(pDiskBuffer+Offset, pBuffer, (int)Count*NUMBYTESPERSECTOR);
		/* And set 'changed' flag */
		EmulationDrives[Drive].bContentsChanged = true;

		return true;
	}

	return false;
}
