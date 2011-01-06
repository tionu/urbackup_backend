/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011  Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "FSImageFactory.h"
#include "../Interface/Server.h"
#include "../Interface/File.h"

#include "fs/ntfs.h"
#include "fs/unknown.h"
#include "vhdfile.h"

IFilesystem *FSImageFactory::createFilesystem(const std::wstring &pDev)
{
	IFile *dev=Server->openFile(pDev, MODE_READ);
	if(dev==NULL)
	{
		Server->Log("Error opening device file", LL_ERROR);
		return NULL;
	}
	char buffer[1024];
	_u32 rc=dev->Read(buffer, 1024);
	if(rc!=1024)
	{
		Server->Log("Error reading data from device", LL_ERROR);
		return NULL;
	}

	Server->destroy(dev);

	if(isNTFS(buffer) )
	{
		Server->Log("Filesystem type is ntfs", LL_DEBUG);
		return new FSNTFS(pDev);
	}
	else
	{
		Server->Log("Unknown filesystem type", LL_DEBUG);
		return new FSUnknown(pDev);
	}
}

bool FSImageFactory::isNTFS(char *buffer)
{
	if(buffer[3]=='N' && buffer[4]=='T' && buffer[5]=='F' && buffer[6]=='S')
	{
		return true;
	}
	else
	{
		return false;
	}
}

void FSImageFactory::destroyFilesystem(IFilesystem *fs)
{
	delete fs;
}

IVHDFile *FSImageFactory::createVHDFile(const std::wstring &fn, bool pRead_only, uint64 pDstsize, unsigned int pBlocksize)
{
	return new VHDFile(fn, pRead_only, pDstsize, pBlocksize);
}

IVHDFile *FSImageFactory::createVHDFile(const std::wstring &fn, const std::wstring &parent_fn, bool pRead_only)
{
	return new VHDFile(fn, parent_fn, pRead_only);
}

void FSImageFactory::destroyVHDFile(IVHDFile *vhd)
{
	delete ((VHDFile*)vhd);
}
