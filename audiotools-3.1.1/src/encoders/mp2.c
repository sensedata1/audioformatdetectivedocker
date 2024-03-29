#include <twolame.h>
#include "../pcmreader.h"

/********************************************************
 Audio Tools, a module and set of tools for manipulating audio data
 Copyright (C) 2007-2015  Brian Langenberger

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*******************************************************/

#define BLOCK_SIZE 4096
#define MP2BUF_SIZE 12320  /*1.25 * BLOCK_SIZE + 7200*/

PyObject*
encoders_encode_mp2(PyObject *dummy, PyObject *args, PyObject *keywds)
{
    char *filename;
    FILE *output_file;
    struct PCMReader *pcmreader;
    static char *kwlist[] = {"filename",
                             "pcmreader",
                             "quality",
                             NULL};
    int quality;
    twolame_options *twolame_opts = NULL;
    int buffer[BLOCK_SIZE * 2];
    short int buffer_l[BLOCK_SIZE];
    short int buffer_r[BLOCK_SIZE];
    unsigned char mp2buf[MP2BUF_SIZE];
    unsigned pcm_frames;
    int to_output;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sO&i",
                                     kwlist,
                                     &filename,
                                     py_obj_to_pcmreader,
                                     &pcmreader,
                                     &quality)) {
        return NULL;
    }

    /*ensure PCMReader object is compatible with MP2 output*/
    if ((pcmreader->channels != 1) && (pcmreader->channels != 2)) {
        PyErr_SetString(PyExc_ValueError, "channel count must be 1 or 2");
        return NULL;
    }

    if (pcmreader->bits_per_sample != 16) {
        PyErr_SetString(PyExc_ValueError, "bits per sample must be 16");
        return NULL;
    }

    /*twolame should resample anything not the proper sample rate*/

    /*open output file for writing*/
    if ((output_file = fopen(filename, "w+b")) == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, filename);
        return NULL;
    }

    /*initialize encoder*/
    if ((twolame_opts = twolame_init()) == NULL) {
        PyErr_SetString(PyExc_ValueError, "unable to initialize twolame");
        goto error;
    }

    /*set stream parameters from PCMReader*/
    twolame_set_in_samplerate(twolame_opts, pcmreader->sample_rate);
    if (pcmreader->channels == 2) {
        twolame_set_num_channels(twolame_opts, 2);
        twolame_set_mode(twolame_opts, TWOLAME_JOINT_STEREO);
    } else if (pcmreader->channels == 1) {
        twolame_set_num_channels(twolame_opts, 1);
        twolame_set_mode(twolame_opts, TWOLAME_MONO);
    }

    /*set bitrate from quality value*/
    twolame_set_bitrate(twolame_opts, quality);

    /*set internal configuration*/
    twolame_init_params(twolame_opts);

    /*for each non-empty FrameList from PCMReader, encode MP2 frame*/
    while ((pcm_frames = pcmreader->read(pcmreader, BLOCK_SIZE, buffer)) > 0) {
        unsigned i;
        if (pcmreader->channels == 2) {
            for (i = 0; i < pcm_frames; i++) {
                buffer_l[i] = (short int)buffer[i * 2];
                buffer_r[i] = (short int)buffer[i * 2 + 1];
            }
        } else {
            for (i = 0; i < pcm_frames; i++) {
                buffer_l[i] = buffer_r[i] = (short int)buffer[i];
            }
        }

        if ((to_output = twolame_encode_buffer(twolame_opts,
                                               buffer_l,
                                               buffer_r,
                                               pcm_frames,
                                               mp2buf,
                                               MP2BUF_SIZE)) >= 0) {
            fwrite(mp2buf, sizeof(unsigned char), to_output, output_file);
        } else {
            PyErr_SetString(PyExc_ValueError, "error encoding MP2 frame");
            goto error;
        }
    }

    if (pcmreader->status != PCM_OK) {
        PyErr_SetString(PyExc_IOError, "I/O error from pcmreader");
        goto error;
    }

    /*flush remaining MP2 data*/
    to_output = twolame_encode_flush(twolame_opts, mp2buf, MP2BUF_SIZE);
    fwrite(mp2buf, sizeof(unsigned char), to_output, output_file);

    /*free internal TwoLAME structures*/
    if (twolame_opts != NULL)
        twolame_close(&twolame_opts);
    fclose(output_file);
    pcmreader->del(pcmreader);
    Py_INCREF(Py_None);
    return Py_None;

error:
    /*free internal TwoLAME structures*/
    if (twolame_opts != NULL)
        twolame_close(&twolame_opts);
    fclose(output_file);
    pcmreader->del(pcmreader);
    return NULL;
}
