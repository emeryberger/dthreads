#ifndef __OBJECTHEADER_H__
#define __OBJECTHEADER_H__

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file  objectheader.h
 * @brief Heap object header.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

class objectHeader {
public:
	enum {
		MAGIC = 0xCAFEBABE
	};

	objectHeader(size_t sz) :
		_size(sz), _magic(MAGIC) {
	}

	size_t getSize(void) {
		sanityCheck();
		return _size;
	}

	bool isValidObject(void) {
		bool ret = false;
		ret = sanityCheck();
		if (ret == true) {
			//	fprintf(stderr, "isValidObject with size %d\n", _size);
			// FIXME: _size should be multiple of 8s according to our heap implementation.
			ret = (_size % 8 == 0) ? true : false;
		}
		return ret;
	}

	bool verifyMagic(void) {
		if (_magic == MAGIC)
			return (true);
		else
			return (false);
	}

private:
	bool sanityCheck(void) {
		if (_magic != MAGIC) {
			fprintf(stderr, "HLY FK in %d. Current _magic %Zx at %p\n", getpid(), _magic, &_magic);
			::abort();
		}
		return true;
	}

	size_t _magic;
	size_t _size;
};

#endif /* __OBJECTHEADER_H__ */
