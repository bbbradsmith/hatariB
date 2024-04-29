/*
  Hatari - st.h

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.
*/

extern bool ST_FileNameIsST(const char *pszFileName, bool bAllowGZ);
extern uint8_t *ST_ReadDisk(int Drive, const char *pszFileName, long *pImageSize, int *pImageType);
extern bool ST_WriteDisk(int Drive, const char *pszFileName, uint8_t *pBuffer, int ImageSize);
