/***********************************************************************/ /**
 * @file config.c:
 *
 * File oriented utility routines to use with a DataLink connection
 * description.
 *
 * This file is part of the DataLink Library.
 *
 * Copyright (c) 2023 Chad Trabant, EarthScope Data Services
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "libdali.h"
#include "portable.h"

/***********************************************************************/ /**
 * @brief Create a compound regular expression from a list in a file
 *
 * Read a list of stream regular expressions from a file and create a
 * compound regular expression.  The caller is responsible for
 * free'ing the returned string.
 *
 * @return A composite regex pattern on success and NULL on error.
 ***************************************************************************/
char *
dl_read_streamlist (DLCP *dlconn, const char *streamfile)
{
  char *regex = 0;
  char line[100];
  char *ptr;
  int streamfd;
  int count = 0;
  int idx;

  /* Open the stream list file */
  if ((streamfd = dlp_openfile (streamfile, 'r')) < 0)
  {
    if (errno == ENOENT)
    {
      dl_log_r (dlconn, 2, 0, "could not find stream list file: %s\n", streamfile);
      return NULL;
    }
    else
    {
      dl_log_r (dlconn, 2, 0, "opening stream list file, %s\n", strerror (errno));
      return NULL;
    }
  }

  dl_log_r (dlconn, 1, 1, "Reading list of streams from %s\n", streamfile);

  while ((dl_readline (streamfd, line, sizeof (line))) >= 0)
  {
    ptr = line;

    /* Trim initial white space */
    while (isspace ((int)*ptr))
      ptr++;

    /* Trim trailing white space */
    idx = strlen (ptr) - 1;
    while (idx >= 0 && isspace ((int)ptr[idx]))
      ptr[idx--] = '\0';

    /* Ignore blank or comment lines */
    if (strlen (ptr) == 0 || ptr[0] == '#' || ptr[0] == '*')
      continue;

    /* Add this stream to the stream chain */
    if (dl_addtostring (&regex, ptr, "|", MAXREGEXSIZE))
    {
      dl_log_r (dlconn, 2, 0, "no streams defined in %s\n", streamfile);
      return NULL;
    }

    count++;
  }

  if (count == 0)
  {
    dl_log_r (dlconn, 2, 0, "no streams defined in %s\n", streamfile);
  }
  else if (count > 0)
  {
    dl_log_r (dlconn, 1, 2, "Read %d streams from %s\n", count, streamfile);
  }

  if (close (streamfd))
  {
    dl_log_r (dlconn, 2, 0, "closing stream list file, %s\n", strerror (errno));
    return NULL;
  }

  return regex;
} /* End of dl_read_streamlist() */
