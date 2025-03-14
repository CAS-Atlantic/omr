/*******************************************************************************
 * Copyright IBM Corp. and others 1991
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

/**
 * @file
 * @ingroup Port
 * @brief file
 */
#include "omrport.h"



/**
 * Read a line of text from the file into buf.
 * Character conversion is only done on z/OS (from platform encoding to UTF-8)
 * This is mostly equivalent to fgets in standard C.
 *
 * @param[in] portLibrary The port library
 * @param[in] fd File descriptor.
 * @param[in,out] buf Buffer for read in text.
 * @param[in] nbytes Size of buffer.
 *
 * @return buf on success, NULL on failure.
 */
char *
omrfile_read_text(struct OMRPortLibrary *portLibrary, intptr_t fd, char *buf, intptr_t nbytes)
{
	char *cursor = buf;

	if (nbytes <= 0) {
		return NULL;
	}

	/* discount 1 for the trailing NUL */
	nbytes -= 1;

	while (nbytes > 0) {
		char temp[64];
		intptr_t size = sizeof(temp) > nbytes ? nbytes : sizeof(temp);
		intptr_t count = portLibrary->file_read(portLibrary, fd, temp, size);
		intptr_t i = 0;

		/* ignore translation for now */
		if (count < 0) {
			if (cursor == buf) {
				return NULL;
			} else {
				break;
			}
		}

		for (i = 0; i < count; i++) {
			char c = temp[i];
			*cursor++ = c;

			if (c == '\n') { /* EOL */
				portLibrary->file_seek(portLibrary, fd, i - count + 1, EsSeekCur);
				*cursor = '\0';
				return buf;
			}
		}
		nbytes -= count;
	}

	*cursor = '\0';
	return buf;
}

/**
 * Output the buffer onto the stream as text. The buffer is a UTF8-encoded array of chars.
 * It is converted to the appropriate platform encoding.
 *
 * @param[in] portLibrary The port library
 * @param[in] fd the file descriptor.
 * @param[in] buf buffer of text to be output.
 * @param[in] nbytes size of buffer of text to be output.
 *
 * @return 0 on success, negative error code on failure.
 */
int32_t
omrfile_write_text(struct OMRPortLibrary *portLibrary, intptr_t fd, const char *buf, uintptr_t nbytes)
{
	intptr_t result = portLibrary->file_write(portLibrary, fd, (void *)buf, nbytes);
	return (result == nbytes) ? 0 : result;
}


/**
 * Retrieve the character set name that file_write_text will encode strings into
 * during the UTF-8 to native transformation. The character set itself is returned as UTF-8.
 *
 * @param[in] portLibrary the port library
 * @param[out] charsetName a caller-allocated buffer that we will write the name of the character set to.
 *                       In practice, this string will be generally less than 64 chars but this cannot be guaranteed.
 * @param[in] nbytes the size of charsetName. A terminating null is appended, so on success at most
 *                       nbytes-1 will be written.
 *
 * @return 0 on success, negative on failure, positive return code when character set is available but
 *               charsetName was too short to store. The required space is given by the return code.
 */
int32_t
omrfile_get_text_encoding(struct OMRPortLibrary *portLibrary, char *charsetName, uintptr_t nbytes)
{
	if (NULL == buf) {
		return -1;
	} else {
		uintptr_t length = strlen("CP850");

		/* CP850 unless overridden because:
		 * 1. Anything in a valid Java identifier in CP437, ASCII, and ANSI X3.4-1986 maps directly into CP850.
		 * 2. *Most* CP1252 characters in Java identifiers have the same code point in CP850.
		 * 3. If the current platform doesn't provide an override, code pages really aren't Problem #1.
		 */
		if (nbytes <= length) {
			return (int32_t)(length + 1);
		}
		strcpy(charsetName, "CP850");
	}
	return 0;
}
