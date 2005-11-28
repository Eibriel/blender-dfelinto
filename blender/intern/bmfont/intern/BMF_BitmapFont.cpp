/**
 * $Id$
 * ***** BEGIN GPL/BL DUAL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version. The Blender
 * Foundation also sells licenses for use in proprietary software under
 * the Blender License.  See http://www.blender.org/BL/ for information
 * about this.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 *
 * The Original Code is: all of this file.
 *
 * Contributor(s): none yet.
 *
 * ***** END GPL/BL DUAL LICENSE BLOCK *****
 */

/**

 * $Id$
 * Copyright (C) 2001 NaN Technologies B.V.
 */

#include <string.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(WIN32) || defined(__APPLE__)
	#ifdef WIN32
        #if !defined(__CYGWIN32__)
        #pragma warning(disable:4244)
        #endif /* __CYGWIN32__ */
		#include <windows.h>
		#include <GL/gl.h>
	#else // WIN32
		// __APPLE__ is defined
		#include <AGL/gl.h>
	#endif // WIN32
#else // defined(WIN32) || defined(__APPLE__)
	#include <GL/gl.h>
#endif // defined(WIN32) || defined(__APPLE__)

#include "BMF_BitmapFont.h"


#ifdef __APPLE__	 
#include <stdio.h>

static int needs_nvidia_rasterpos_workaround(void)
{
	static int well_is_it= -1;
	
	if (well_is_it==-1)
	{
		well_is_it= (strncmp((char *)glGetString(GL_RENDERER), "NVIDIA GeForce 6800", 18) == 0);
		if ( well_is_it != 0)
		{
			const GLubyte* vers = glGetString(GL_VERSION);
			const GLubyte* v = vers;
			int major = 0, minor = 0, sub = 0;
			
			//advance to the '-'
			while ((*v != 0) && (*v!='-'))
				v++;
			
			if (*v == '-')
			{
				int i = 0;
				v++;
				
				while ((v[i] <= '9') && (v[i] >= '0'))
				{
					major *=10;
					major += v[i]-'0';
					i++;
				}
				
				if (v[i] == '.')
				{
					i++;
					while ((v[i] <= '9') && (v[i] >= '0'))
					{
						minor *=10;
						minor += v[i]-'0';
						i++;
					}
				}
				else
					major = -1;
				
				if (v[i] == '.')
				{
					i++;
					while ((v[i] <= '9') && (v[i] >= '0'))
					{
						sub *=10;
						sub += v[i]-'0';
						i++;
					}
				}
				else
					minor = -1;
			}

			//OS X 10.4.3 is the first version that contained the fix for this problem
			// and the 6800's driver version in it is 1.4.16.  So anything after that
			// doesn't need the workaround

			if ( (major == -1) || (minor == -1))
			//If anything went wrong don't do the workaround
			//
				well_is_it = 0;
			else if ( (major <= 1) && (minor <= 4) && (sub < 16))
				well_is_it = 1;
			else
				well_is_it = 0;
		}	
	}

	return well_is_it;
}


#endif

BMF_BitmapFont::BMF_BitmapFont(BMF_FontData* fontData)
: m_fontData(fontData)
{
}


BMF_BitmapFont::~BMF_BitmapFont(void)
{
}

void BMF_BitmapFont::DrawString(char* str)
{
	GLint alignment;
	unsigned char c;

#ifdef __APPLE__	 
     GLint vp[4];  // hack stuff	 
     GLubyte nullm = 0;      // hack stuff	 
 	 
     if(needs_nvidia_rasterpos_workaround()) {	 // was is_a_really_crappy_nvidia_card()
             glGetIntegerv(GL_VIEWPORT, vp);   // hack stuff	 
 	 
             glBitmap(1, 1, 0, 0, -vp[0], vp[1], &nullm);	 
 	 
         }	 
 #endif
 
 	glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	while ( (c = (unsigned char) *str++) ) {
		BMF_CharData & cd = m_fontData->chars[c];
		
		if (cd.data_offset==-1) {
			GLubyte nullBitmap = 0;
		
			glBitmap(1, 1, 0, 0, cd.advance, 0, &nullBitmap);	
		} else {
			GLubyte *bitmap = &m_fontData->bitmap_data[cd.data_offset];
		
			glBitmap(cd.width, cd.height, cd.xorig, cd.yorig, cd.advance, 0, bitmap);
		}
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}


int BMF_BitmapFont::GetStringWidth(char* str)
{
	unsigned char c;
	int length = 0;

	while ( (c = (unsigned char) *str++) ) {
		length += m_fontData->chars[c].advance;
	}
	
	return length;
}

void BMF_BitmapFont::GetBoundingBox(int & xMin, int & yMin, int & xMax, int & yMax)
{
	xMin = m_fontData->xmin;
	yMin = m_fontData->ymin;
	xMax = m_fontData->xmax;
	yMax = m_fontData->ymax;
}

int BMF_BitmapFont::GetTexture()
{
	int fWidth = m_fontData->xmax - m_fontData->xmin;
	int fHeight = m_fontData->ymax - m_fontData->ymin;
	
	if (fWidth>=16 || fHeight>=16) {
		return -1;
	}
	
	int cRows = 16, cCols = 16;
	int cWidth = 16, cHeight = 16;
	int iWidth = cCols*cWidth;
	int iHeight = cRows*cHeight;
	GLubyte *img = new GLubyte [iHeight*iWidth];
	GLuint texId;

	int baseLine = -(m_fontData->ymin);
	
	memset(img, 0, iHeight*iWidth);
	for (int i = 0; i<256; i++) {
		BMF_CharData & cd = m_fontData->chars[i];
		
		if (cd.data_offset != -1) {
			int cellX = i%16;
			int cellY = i/16;
			
			for (int y = 0; y<cd.height; y++) {
				GLubyte* imgRow = &img[(cellY*cHeight + y + baseLine - cd.yorig)*iWidth];
				GLubyte* chrRow = &m_fontData->bitmap_data[cd.data_offset + ((cd.width+7)/8)*y];

				for (int x = 0; x<cd.width; x++) {
					GLubyte* imgPxl = &imgRow[(cellX*cWidth + x - cd.xorig)];
					int byteIdx = x/8;
					int bitIdx = 7 - (x%8);
					
					if (chrRow[byteIdx]&(1<<bitIdx)) {
						imgPxl[0] = 255;
					}
				}
			}
		}
	}
	
	glGenTextures(1, &texId);
	
	glBindTexture(GL_TEXTURE_2D, texId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA4, iWidth, iHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, img);
	if (glGetError()) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE4_ALPHA4, iWidth, iHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, img);
	}
	
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	delete [] img;
	
	return texId;
}

void BMF_BitmapFont::DrawStringTexture(char *str, float x, float y, float z)
{
	unsigned char c;
	float pos = 0;
	
	int baseLine = -(m_fontData->ymin);

	glBegin(GL_QUADS);
	while ( (c = (unsigned char) *str++) ) {
		BMF_CharData & cd = m_fontData->chars[c];
		
		if (cd.data_offset != -1) {
			float cellX = (c%16)/16.0;
			float cellY = (c/16)/16.0;
		
			glTexCoord2f(cellX + 1.0/16.0, cellY);
			glVertex3f(x + pos + 16.0, -baseLine + y + 0.0, z);

			glTexCoord2f(cellX + 1.0/16.0, cellY + 1.0/16.0);
			glVertex3f(x + pos + 16.0, -baseLine + y + 16.0, z);

			glTexCoord2f(cellX, cellY + 1.0/16.0);
			glVertex3f(x + pos + 0.0, -baseLine + y + 16.0, z);

			glTexCoord2f(cellX, cellY);
			glVertex3f(x + pos + 0.0, -baseLine + y + 0.0, z);
		}
		
		pos += cd.advance;
	}
	glEnd();
}
