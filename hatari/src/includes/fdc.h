/*
  Hatari - fdc.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

#ifndef HATARI_FDC_H
#define HATARI_FDC_H


/* Values for the Size byte in the Address Field of a sector */
#define	FDC_SECTOR_SIZE_MASK			0x03		/* Only bits 0-1 of the Sector size in the ID field are used by the WD1772 */

#define	FDC_SECTOR_SIZE_128			0		/* Sector size used in the ID fields */
#define	FDC_SECTOR_SIZE_256			1
#define	FDC_SECTOR_SIZE_512			2
#define	FDC_SECTOR_SIZE_1024			3


/* These are some standard GAP values to format a track with 9 or 10 sectors */
/* When handling ST/MSA disk images, those values are required to get accurate */
/* timings when emulating disk's spin and index's position. */
/* Those values are also use to build standard sector in STX disk images when */
/* track contains only the sector data and no sector info. */
#define	FDC_TRACK_LAYOUT_STANDARD_GAP1		60		/* Track Pre GAP : 0x4e */
#define	FDC_TRACK_LAYOUT_STANDARD_GAP2		12		/* Sector ID Pre GAP : 0x00 */
#define	FDC_TRACK_LAYOUT_STANDARD_GAP3a		22		/* Sector ID Post GAP : 0x4e */
#define	FDC_TRACK_LAYOUT_STANDARD_GAP3b		12		/* Sector DATA Pre GAP : 0x00 */
#define	FDC_TRACK_LAYOUT_STANDARD_GAP4		40		/* Sector DATA Pre GAP : 0x4e */
#define	FDC_TRACK_LAYOUT_STANDARD_GAP5		0		/* Track Post GAP : 0x4e (to fill the rest of the track, value is variable) */
								/* GAP5 is 664 bytes for 9 sectors or 50 bytes for 10 sectors */

/* Size of a raw standard 512 byte sector in a track, including ID field and all GAPs : 614 bytes */
/* (this must be the same as the data returned in FDC_UpdateReadTrackCmd() ) */
#define	FDC_TRACK_LAYOUT_STANDARD_RAW_SECTOR_512	( FDC_TRACK_LAYOUT_STANDARD_GAP2 \
				+ 3 + 1 + 6 + FDC_TRACK_LAYOUT_STANDARD_GAP3a + FDC_TRACK_LAYOUT_STANDARD_GAP3b \
				+ 3 + 1 + 512 + 2 + FDC_TRACK_LAYOUT_STANDARD_GAP4 )


#define	FDC_IRQ_SOURCE_COMPLETE			(1<<0)		/* IRQ set after completing a command */
#define	FDC_IRQ_SOURCE_INDEX			(1<<1)		/* IRQ set when COND_IP is set and index is reached */
#define	FDC_IRQ_SOURCE_FORCED			(1<<2)		/* IRQ was forced by a previous Dx command with COND_IMMEDIATE */
#define	FDC_IRQ_SOURCE_HDC			(1<<3)		/* IRQ set by HDC */
#define	FDC_IRQ_SOURCE_OTHER			(1<<4)		/* IRQ set by other parts (IPF) */


/* Option bits in the command register */
#define	FDC_COMMAND_BIT_VERIFY			(1<<2)		/* 0=no verify after type I, 1=verify after type I */
#define	FDC_COMMAND_BIT_HEAD_LOAD		(1<<2)		/* for type II/III 0=no extra delay, 1=add 30 ms delay to set the head */
#define	FDC_COMMAND_BIT_SPIN_UP			(1<<3)		/* 0=enable motor's spin up, 1=disable motor's spin up */
#define	FDC_COMMAND_BIT_UPDATE_TRACK		(1<<4)		/* 0=don't update TR after type I, 1=update TR after type I */
#define	FDC_COMMAND_BIT_MULTIPLE_SECTOR		(1<<4)		/* 0=read/write only 1 sector, 1=read/write many sectors */


#define	FDC_INTERRUPT_COND_IP			(1<<2)		/* Force interrupt on Index Pulse */
#define	FDC_INTERRUPT_COND_IMMEDIATE		(1<<3)		/* Force interrupt immediate */


#define	FDC_DC_SIGNAL_EJECTED			0
#define	FDC_DC_SIGNAL_INSERTED			1


extern int	FDC_StepRate_ms[];


extern void	FDC_MemorySnapShot_Capture ( bool bSave );
extern void	FDC_Init ( void );
extern void	FDC_Reset ( bool bCold );
extern void	FDC_SetDMAStatus ( bool bError );

extern void	FDC_SetIRQ ( uint8_t IRQ_Source );
extern void	FDC_ClearIRQ ( void );
extern void	FDC_ClearHdcIRQ(void);
extern void	FDC_InterruptHandler_Update ( void );

extern void	FDC_Drive_Set_BusyLed ( uint8_t SR );
extern int	FDC_Get_Statusbar_Text ( char *text, size_t maxlen );
extern void	FDC_Drive_Set_Enable ( int Drive , bool value );
extern void	FDC_Drive_Set_NumberOfHeads ( int Drive , int NbrHeads );
extern void	FDC_InsertFloppy ( int Drive );
extern void	FDC_EjectFloppy ( int Drive );
extern void	FDC_SetDriveSide ( uint8_t io_porta_old , uint8_t io_porta_new );
extern int	FDC_GetBytesPerTrack ( uint8_t Drive , uint8_t Track , uint8_t Side );
extern int	FDC_GetFloppyDensity ( uint8_t Drive );
extern int	FDC_MachineHandleDensity ( uint8_t Drive );

extern int	FDC_IndexPulse_GetCurrentPos_FdcCycles ( uint32_t *pFdcCyclesPerRev );
extern int	FDC_IndexPulse_GetCurrentPos_NbBytes ( void );
extern int	FDC_IndexPulse_GetState ( void );
extern int	FDC_NextIndexPulse_FdcCycles ( void );

extern uint8_t	FDC_GetCmdType ( uint8_t CR );

extern void	FDC_DiskController_WriteWord ( void );
extern void	FDC_DiskControllerStatus_ReadWord ( void );
extern void	FDC_DmaModeControl_WriteWord ( void );
extern void	FDC_DmaStatus_ReadWord ( void );
extern int	FDC_DMA_GetModeControl_R_WR ( void );
extern int	FDC_DMA_GetMode(void);
extern void	FDC_DMA_FIFO_Push ( uint8_t Byte );
extern uint8_t	FDC_DMA_FIFO_Pull ( void );
extern int	FDC_DMA_GetSectorCount ( void );

extern void	FDC_Buffer_Reset ( void );
extern void	FDC_Buffer_Add_Timing ( uint8_t Byte , uint16_t Timing );
extern void	FDC_Buffer_Add ( uint8_t Byte );
extern uint16_t	FDC_Buffer_Read_Timing ( void );
extern uint8_t	FDC_Buffer_Read_Byte ( void );
extern uint8_t	FDC_Buffer_Read_Byte_pos ( int pos );
extern int	FDC_Buffer_Get_Size ( void );

extern void	FDC_DmaAddress_ReadByte ( void );
extern void	FDC_DmaAddress_WriteByte ( void );
extern uint32_t	FDC_GetDMAAddress ( void );
extern void	FDC_WriteDMAAddress ( uint32_t Address );

extern void	FDC_DensityMode_WriteWord ( void );
extern void	FDC_DensityMode_ReadWord ( void );

#endif /* ifndef HATARI_FDC_H */
