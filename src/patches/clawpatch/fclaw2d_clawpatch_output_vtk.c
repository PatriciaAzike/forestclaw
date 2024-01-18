/*
Copyright (c) 2012-2022 Carsten Burstedde, Donna Calhoun, Scott Aiton
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef REFINE_DIM
#define REFINE_DIM 2
#endif

#ifndef PATCH_DIM
#define PATCH_DIM 2
#endif

/** Round up quotient x / y. */
#define FCLAW_VTK_CEIL(x, y) (((x) + (y) - 1) / (y)) /**< round up the quotient
                                                        x / y; this is a local
                                                        macro used for
                                                        computations regarding
                                                        the VTK patch buffering */

#if REFINE_DIM == 2 && PATCH_DIM == 2

#include <fclaw2d_clawpatch_output_vtk.h>

#include <fclaw2d_clawpatch.h>
#include <fclaw2d_clawpatch_options.h>

#define PATCH_CHILDREN 4

#elif REFINE_DIM == 2 && PATCH_DIM == 3

#include <fclaw3dx_clawpatch_output_vtk.h>

#include <fclaw3dx_clawpatch.h>
#include <fclaw3dx_clawpatch_options.h>

#include <_fclaw2d_to_fclaw3dx.h>

#define PATCH_CHILDREN 8

#else
#error "This combination of REFINE_DIM and PATCH_DIM is unsupported"
#endif

#include <fclaw2d_global.h>

#include <fclaw2d_options.h>
#include <fclaw2d_map.h>
#include <sc_io.h>

typedef struct fclaw2d_vtk_state
{
    int mx, my;
#if PATCH_DIM == 3
    int mz;
#endif
    int meqn;
    int points_per_patch, cells_per_patch;
    int intsize, ndsize;
    int fits32;
    char filename[BUFSIZ];
    int64_t global_num_points, global_num_cells;
    int64_t global_num_connectivity;
    int64_t offset_position, psize_position;
    int64_t offset_connectivity, psize_connectivity;
    int64_t offset_offsets, psize_offsets;
    int64_t offset_types, psize_types;
    int64_t offset_mpirank, psize_mpirank;
    int64_t offset_blockno, psize_blockno;
    int64_t offset_patchno, psize_patchno;
    int64_t offset_meqn, psize_meqn;
    int64_t offset_end;
    const char *inttype;
    fclaw2d_vtk_patch_data_t coordinate_cb;
    fclaw2d_vtk_patch_data_t value_cb;
    FILE *file;
#ifdef P4EST_ENABLE_MPIIO
    MPI_File mpifile;
    MPI_Offset mpibegin;
#endif
    char *buf;

    /* The following three struct elements are dedicated to manage buffering
     * patches.
     * We track the number of buffered patches in \b num_buffered_patches and
     * if the buffer contains more than \b patch_threshold many patches, the
     * patches are written to disk and the buffer is flushed.
     * The variable \b num_buffered_patches relates to the number of buffered
     * patches during one fclaw2d_vtk_write_field call.
     * If \b patch_threshold is 0, there are no intermediate writes but all
     * patches during a fclaw2d_vtk_write_field are buffered and then written
     * to the disk.
     */
    sc_io_sink_t *sink; /**< the sink to manage the patches buffer */
    int num_buffered_patches; /**< the number of patches buffered during the
                                   current fclaw2d_vtk_write_field call */
    int patch_threshold; /**< maximal number of patches that are buffered during
                              a call of fclaw2d_vtk_write_field */
}
fclaw2d_vtk_state_t;

static int
fclaw2d_vtk_write_header (fclaw2d_domain_t * domain, fclaw2d_vtk_state_t * s)
{
    int retval;
    FILE *file;

    /* unconditionally open the file */
    file = fopen (s->filename, "wb");
    if (file == NULL)
    {
        return -1;
    }
    s->file = file;

    /* stop writing after first unsuccessful operation */
    retval = 0;
    retval = retval || fprintf (file, "<?xml version=\"1.0\"?>\n") < 0;
    retval = retval || fprintf (file, "<VTKFile type=\"UnstructuredGrid\" "
                                "version=\"0.1\" "
                                "byte_order=\"LittleEndian\" "
                                "header_type=\"UInt64\" "
                                ">\n") < 0;
    retval = retval || fprintf (file, " <UnstructuredGrid>\n") < 0;
    retval = retval || fprintf (file, "  <Piece NumberOfPoints=\"%lld\" "
                                "NumberOfCells=\"%lld\">\n",
                                (long long) s->global_num_points,
                                (long long) s->global_num_cells) < 0;
    retval = retval || fprintf (file, "   <Points>\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"Float64\" "
                                "Name=\"Position\" NumberOfComponents=\"3\" "
                                "format=\"appended\" offset=\"%lld\">\n",
                                (long long) s->offset_position) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "   </Points>\n") < 0;
    retval = retval || fprintf (file, "   <Cells>\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"%s\" "
                                "Name=\"connectivity\" format=\"appended\" "
                                "offset=\"%lld\">\n", s->inttype,
                                (long long) s->offset_connectivity) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"%s\" "
                                "Name=\"offsets\" format=\"appended\" "
                                "offset=\"%lld\">\n", s->inttype,
                                (long long) s->offset_offsets) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"UInt8\" "
                                "Name=\"types\" format=\"appended\" "
                                "offset=\"%lld\">\n",
                                (long long) s->offset_types) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "   </Cells>\n") < 0;
    retval = retval || fprintf (file, "   <CellData Scalars=\"mpirank,"
                                "blockno,patchno\" Fields=\"meqn\">\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"Int32\" "
                                "Name=\"mpirank\" format=\"appended\" "
                                "offset=\"%lld\">\n",
                                (long long) s->offset_mpirank) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"Int32\" "
                                "Name=\"blockno\" format=\"appended\" "
                                "offset=\"%lld\">\n",
                                (long long) s->offset_blockno) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "    <DataArray type=\"%s\" "
                                "Name=\"patchno\" format=\"appended\" "
                                "offset=\"%lld\">\n", s->inttype,
                                (long long) s->offset_patchno) < 0;
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    if (s->meqn == 1)
    {
        retval = retval || fprintf (file, "    <DataArray type=\"Float32\" "
                                    "Name=\"meqn\" "
                                    "format=\"appended\" offset=\"%lld\">\n",
                                    (long long) s->offset_meqn) < 0;
    }
    else
    {
        retval = retval || fprintf (file, "    <DataArray type=\"Float32\" "
                                    "Name=\"meqn\" NumberOfComponents=\"%d\" "
                                    "format=\"appended\" offset=\"%lld\">\n",
                                    s->meqn, (long long) s->offset_meqn) < 0;
    }
    retval = retval || fprintf (file, "    </DataArray>\n") < 0;
    retval = retval || fprintf (file, "   </CellData>\n") < 0;
    retval = retval || fprintf (file, "   <PointData>\n") < 0;
    retval = retval || fprintf (file, "   </PointData>\n") < 0;
    retval = retval || fprintf (file, "  </Piece>\n") < 0;
    retval = retval || fprintf (file, " </UnstructuredGrid>\n") < 0;
    retval = retval || fprintf (file, " <AppendedData "
                                "encoding=\"raw\">\n  _") < 0;

#ifdef P4EST_ENABLE_MPIIO
    /* unconditionally close the file when in MPI I/O mode */
    retval = fclose (file) || retval;
    s->file = NULL;
#endif

    return retval ? -1 : 0;
}

/** This function adds to a buffer and writes to file if a threshold is exceeded.
 *
 * This function assumes that the buffer consists of patch data of the size
 * \b psize_field. This means it must hold
 * \b s->sink->buffer_bytes % \b psize_field == 0.
 *
 * \param [in,out]  s               The VTK state.
 * \param [in]      psize_field     The number of bytes, which are intended to
 *                                  add to the buffer.
 * \param [in]      threshold       If the number of patches, which are already
 *                                  stored in the buffer + 1 is greater than
 *                                  \b threshold, the buffer is flushed to disk.
 *                                  Then the new patch is added to the buffer.
 *                                  0 means that there is no threshold and
 *                                  the data just added to \b s->sink.
 */
static void
add_to_buffer (fclaw2d_vtk_state_t * s, int64_t psize_field)
{
    FCLAW_ASSERT (s != NULL);

    int mpiret;
#ifdef P4EST_ENABLE_MPIIO
    MPI_Status mpistatus;
#else
    size_t retvalz;
#endif
    FCLAW_ASSERT (s->patch_threshold >= 0);
    FCLAW_ASSERT (((int) s->sink->buffer_bytes) % psize_field == 0);

    if (s->patch_threshold != 0
        && (s->num_buffered_patches + 1 > s->patch_threshold))
    {
        FCLAW_ASSERT ((int) s->sink->buffer_bytes >= psize_field);
        FCLAW_ASSERT (((int) s->sink->buffer_bytes) / psize_field ==
                      s->num_buffered_patches);
        /* buffer threshold exceeded */
        /* write the current buffer to disk */
#ifdef P4EST_ENABLE_MPIIO
        if (s->num_buffered_patches > 0)
        {
            mpiret =
                MPI_File_write_all (s->mpifile, s->sink->buffer->array,
                                    (int) s->sink->buffer_bytes, MPI_BYTE,
                                    &mpistatus);
            SC_CHECK_MPI (mpiret);
        }
#else
        retvalz =
            fwrite (s->sink->buffer->array, s->sink->buffer_bytes, 1,
                    s->file);
        SC_CHECK_ABORT (retvalz == 1, "VTK file write failed");
#endif
        /* reset the sink */
        mpiret = sc_io_sink_complete (s->sink, NULL, NULL);
        SC_CHECK_ABORT (mpiret == 0, "VTK buffer completion failed");
        /* reset the buffer */
        sc_array_reset (s->sink->buffer);
        s->sink->buffer_bytes = 0;
        s->num_buffered_patches = 0;
    }

    mpiret = sc_io_sink_write (s->sink, s->buf, (size_t) psize_field);
    SC_CHECK_ABORT (mpiret == 0, "VTK buffer write failed");
    ++s->num_buffered_patches;
}

static void
write_position_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                   int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;

    s->coordinate_cb (g->glob, patch, blockno, patchno, s->buf);
    add_to_buffer (s, s->psize_position);
}

static void
write_connectivity_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                       int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;
    int i, j;
    const int64_t pbefore = s->points_per_patch *
        (domain->global_num_patches_before +
         domain->blocks[blockno].num_patches_before + patchno);

    if (s->fits32)
    {
        int32_t *idata = (int32_t *) s->buf;
        int32_t l;

#if PATCH_DIM == 3
        int k;
        for (k = 0; k < s->mz; ++k)
        {
#if 0
        }
#endif
#endif
        for (j = 0; j < s->my; ++j)
        {
            for (i = 0; i < s->mx; ++i)
            {
                l = (int32_t) pbefore + i + j * (s->mx + 1)
#if PATCH_DIM == 3
                     + k * (s->my + 1) * (s->mx + 1)
#endif
                     + 0;
                *idata++ = l;
                *idata++ = l + 1;
                *idata++ = l + (s->mx + 2);
                *idata++ = l + (s->mx + 1);
#if PATCH_DIM == 3
                *idata++ = l + (s->mx + 1) * (s->my + 1);
                *idata++ = l + (s->mx + 1) * (s->my + 1) + 1;
                *idata++ = l + (s->mx + 1) * (s->my + 1) + (s->mx + 2);
                *idata++ = l + (s->mx + 1) * (s->my + 1) + (s->mx + 1);
#endif
            }
        }
#if PATCH_DIM == 3
#if 0
        {
#endif
        }
#endif
    }
    else
    {
        int64_t *idata = (int64_t *) s->buf;
        int64_t l;

#if PATCH_DIM == 3
        int k;
        for (k = 0; k < s->mz; ++k)
        {
#if 0
        }
#endif
#endif
        for (j = 0; j < s->my; ++j)
        {
            for (i = 0; i < s->mx; ++i)
            {
                l = pbefore + i + j * (s->mx + 1)
#if PATCH_DIM == 3
                     + k * (s->my + 1) * (s->mx + 1)
#endif
                     + 0;
                *idata++ = l;
                *idata++ = l + 1;
                *idata++ = l + (s->mx + 2);
                *idata++ = l + (s->mx + 1);
#if PATCH_DIM == 3
                *idata++ = l + (s->mx + 1) * (s->my + 1);
                *idata++ = l + (s->mx + 1) * (s->my + 1) + 1;
                *idata++ = l + (s->mx + 1) * (s->my + 1) + (s->mx + 2);
                *idata++ = l + (s->mx + 1) * (s->my + 1) + (s->mx + 1);
#endif
            }
        }
#if PATCH_DIM == 3
#if 0
        {
#endif
        }
#endif
    }
    add_to_buffer (s, s->psize_connectivity);
}

static void
write_offsets_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                  int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;
    int c;
    const int64_t cbefore = s->cells_per_patch *
        (domain->global_num_patches_before +
         domain->blocks[blockno].num_patches_before + patchno);

    if (s->fits32)
    {
        int32_t *idata = (int32_t *) s->buf;
        int32_t k = PATCH_CHILDREN * (int32_t) (cbefore + 1);
        for (c = 0; c < s->cells_per_patch; k += PATCH_CHILDREN, ++c)
        {
            *idata++ = k;
        }
    }
    else
    {
        int64_t *idata = (int64_t *) s->buf;
        int64_t k = PATCH_CHILDREN * (cbefore + 1);
        for (c = 0; c < s->cells_per_patch; k += PATCH_CHILDREN, ++c)
        {
            *idata++ = k;
        }
    }
    add_to_buffer (s, s->psize_offsets);
}

static void
write_types_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;
    int c;

    char *cdata = s->buf;
    for (c = 0; c < s->cells_per_patch; ++c)
    {
#if PATCH_DIM == 2
        *cdata++ = 9;
#else
        *cdata++ = 12;
#endif
    }
    add_to_buffer (s, s->psize_types);
}

static void
write_mpirank_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                  int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;
    int c;

    int *idata = (int *) s->buf;
    for (c = 0; c < s->cells_per_patch; ++c)
    {
        *idata++ = domain->mpirank;
    }
    add_to_buffer (s, s->psize_mpirank);
}

static void
write_blockno_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                  int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;
    int c;

    int *idata = (int *) s->buf;
    for (c = 0; c < s->cells_per_patch; ++c)
    {
        *idata++ = blockno;
    }
    add_to_buffer (s, s->psize_blockno);
}

static void
write_patchno_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
                  int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t *g = (fclaw2d_global_iterate_t*) user;
    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;
    int c;
    const int64_t gpno =
        domain->global_num_patches_before +
        domain->blocks[blockno].num_patches_before + patchno;

    if (s->fits32)
    {
        const int32_t igpno = (int32_t) gpno;
        int32_t *idata = (int32_t *) s->buf;
        for (c = 0; c < s->cells_per_patch; ++c)
        {
            *idata++ = igpno;
        }
    }
    else
    {
        int64_t *idata = (int64_t *) s->buf;
        for (c = 0; c < s->cells_per_patch; ++c)
        {
            *idata++ = gpno;
        }
    }
    add_to_buffer (s, s->psize_patchno);
}

static void
write_meqn_cb (fclaw2d_domain_t * domain, fclaw2d_patch_t * patch,
               int blockno, int patchno, void *user)
{
    fclaw2d_global_iterate_t* g = (fclaw2d_global_iterate_t*) user;

    fclaw2d_vtk_state_t *s = (fclaw2d_vtk_state_t *) g->user;

    s->value_cb (g->glob, patch, blockno, patchno, s->buf);
    add_to_buffer (s, s->psize_meqn);
}

static void
fclaw2d_vtk_write_field (fclaw2d_global_t * glob, fclaw2d_vtk_state_t * s,
                         int64_t offset_field, int64_t psize_field,
                         fclaw2d_patch_callback_t cb)
{
    fclaw2d_domain_t *domain = glob->domain;

    int64_t bcount;
    sc_array_t buffer;
#ifndef P4EST_ENABLE_MPIIO
    size_t retvalz;
#else
    int i;
    int max_num_writes, local_num_writes;
    int mpiret;
    MPI_Offset mpipos;
#ifdef P4EST_ENABLE_DEBUG
    MPI_Offset mpinew;
#endif
    MPI_Status mpistatus;
#endif

    s->buf = P4EST_ALLOC (char, psize_field);
    /* The buffer array will be resized when required. */
    sc_array_init_size (&buffer, 1, psize_field);
    s->sink = sc_io_sink_new (SC_IO_TYPE_BUFFER, SC_IO_MODE_APPEND,
                              SC_IO_ENCODE_NONE, &buffer);
    FCLAW_ASSERT (s->num_buffered_patches == 0);
    s->num_buffered_patches = 0;
#ifdef P4EST_ENABLE_MPIIO
    mpipos = s->mpibegin + offset_field;
    if (domain->mpirank > 0)
    {
        /* account for byte count */
        mpipos += s->ndsize + psize_field * domain->global_num_patches_before;
    }
    mpiret = MPI_File_seek (s->mpifile, mpipos, MPI_SEEK_SET);
    SC_CHECK_MPI (mpiret);
#endif
    if (domain->mpirank == 0)
    {
        /* write byte count */
        bcount = psize_field * domain->global_num_patches;
#if 0
        P4EST_LDEBUGF ("offset %lld psize %lld bcount %d %o %x\n",
                       (long long) offset_field, (long long) psize_field,
                       bcount, bcount, bcount);
#endif
#ifndef P4EST_ENABLE_MPIIO
        retvalz = fwrite (&bcount, s->ndsize, 1, s->file);
        SC_CHECK_ABORT (retvalz == 1, "VTK file write failed");
#else
        mpiret =
            MPI_File_write (s->mpifile, &bcount, 1, MPI_LONG, &mpistatus);
        SC_CHECK_MPI (mpiret);
#endif
    }
    fclaw2d_global_iterate_patches (glob, cb, s);

#ifdef P4EST_ENABLE_MPIIO
    if (s->num_buffered_patches > 0 || s->patch_threshold == 0)
    {
        /* write the remaining buffered bytes */
        mpiret =
            MPI_File_write_all (s->mpifile, buffer.array,
                                (int) s->sink->buffer_bytes, MPI_BYTE,
                                &mpistatus);
        SC_CHECK_MPI (mpiret);
    }

    if (s->patch_threshold != 0)
    {
        /* Ensure that the collective function MPI_File_write_all is called
         * equally often on each rank.
         */
        max_num_writes =
            FCLAW_VTK_CEIL (glob->domain->local_max_patches,
                            s->patch_threshold);
        local_num_writes =
            FCLAW_VTK_CEIL (glob->domain->local_num_patches,
                            s->patch_threshold);
        FCLAW_ASSERT (max_num_writes - local_num_writes >= 0);

        for (i = 0; i < max_num_writes - local_num_writes; ++i)
        {
            /* This rank has less patches than the rank that holds the maximal
             * number of patches. We compensate this by empty write calls to avoid
             * a deadlock.
             */
            mpiret =
                MPI_File_write_all (s->mpifile, buffer.array, 0, MPI_BYTE,
                                    &mpistatus);
            SC_CHECK_MPI (mpiret);
        }
    }
#else
    retvalz = fwrite (buffer.array, s->sink->buffer_bytes, 1, s->file);
    SC_CHECK_ABORT (retvalz == 1, "VTK file write failed");
#endif

    /* free buffers */
    sc_array_reset (&buffer);
    sc_io_sink_destroy (s->sink);
    P4EST_FREE (s->buf);
    s->num_buffered_patches = 0;

#ifdef P4EST_ENABLE_MPIIO
#ifdef P4EST_ENABLE_DEBUG
    mpiret = MPI_File_get_position (s->mpifile, &mpinew);
    SC_CHECK_MPI (mpiret);
    P4EST_ASSERT (mpinew - mpipos ==
                  (domain->mpirank == 0 ? s->ndsize : 0) +
                  domain->local_num_patches * psize_field);
    P4EST_ASSERT (domain->mpirank < domain->mpisize - 1 ||
                  mpinew - s->mpibegin ==
                  offset_field + s->ndsize +
                  psize_field * domain->global_num_patches);
#endif
#endif
}

static void
fclaw2d_vtk_write_data (fclaw2d_global_t * glob, fclaw2d_vtk_state_t * s)
{
#ifdef P4EST_ENABLE_MPIIO
    int mpiret;
    MPI_Offset mpipos;

    /* collectively open the file in append mode and reserve space */
    mpiret = MPI_File_open (glob->mpicomm, s->filename,
                            MPI_MODE_RDWR | MPI_MODE_APPEND |
                            MPI_MODE_UNIQUE_OPEN, MPI_INFO_NULL, &s->mpifile);
    SC_CHECK_MPI (mpiret);
    mpiret = MPI_File_get_position (s->mpifile, &s->mpibegin);
    SC_CHECK_MPI (mpiret);
    mpipos = s->mpibegin + (MPI_Offset) s->offset_end;
    mpiret = MPI_File_preallocate (s->mpifile, mpipos);
    SC_CHECK_MPI (mpiret);
#endif

    /* write meta data fields */
    fclaw2d_vtk_write_field (glob, s, s->offset_position, s->psize_position,
                             write_position_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_connectivity,
                             s->psize_connectivity, write_connectivity_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_offsets, s->psize_offsets,
                             write_offsets_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_types, s->psize_types,
                             write_types_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_mpirank, s->psize_mpirank,
                             write_mpirank_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_blockno, s->psize_blockno,
                             write_blockno_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_patchno, s->psize_patchno,
                             write_patchno_cb);
    fclaw2d_vtk_write_field (glob, s, s->offset_meqn, s->psize_meqn,
                             write_meqn_cb);

#ifdef P4EST_ENABLE_MPIIO
    /* collectively close the file */
    mpiret = MPI_File_close (&s->mpifile);
    SC_CHECK_MPI (mpiret);
#endif
}

static int
fclaw2d_vtk_write_footer (fclaw2d_domain_t * domain, fclaw2d_vtk_state_t * s)
{
    int retval;
    FILE *file;

#ifndef P4EST_ENABLE_MPIIO
    file = s->file;
#else
    /* unconditionally open the file */
    file = fopen (s->filename, "ab");
    if (file == NULL)
    {
        return -1;
    }
    s->file = file;
#endif

    /* stop writing after first unsuccessful operation */
    retval = 0;
    retval = retval || fprintf (file, "\n </AppendedData>\n</VTKFile>\n") < 0;

    /* unconditionally close the file */
    retval = fclose (file) || retval;
    s->file = NULL;

    return retval ? -1 : 0;
}

int
fclaw2d_vtk_write_file (fclaw2d_global_t * glob, const char *basename,
                        int mx, int my,
#if PATCH_DIM == 3
                        int mz,
#endif
                        int meqn,
                        double vtkspace, int vtkwrite,
                        fclaw2d_vtk_patch_data_t coordinate_cb,
                        fclaw2d_vtk_patch_data_t value_cb,
                        int patch_threshold)
{
    fclaw2d_domain_t *domain = glob->domain;

    FCLAW_ASSERT (patch_threshold >= 0);

    int retval, gretval;
    int mpiret;
    fclaw2d_vtk_state_t ps, *s = &ps;

    /* set up VTK internal information */
    s->mx = mx;
    s->my = my;
#if PATCH_DIM == 3
    s->mz = mz;
#endif
    s->meqn = meqn;
    s->points_per_patch = (mx + 1) * (my + 1);
    s->cells_per_patch = mx * my;
#if PATCH_DIM == 3
    s->points_per_patch *= (mz + 1);
    s->cells_per_patch *= mz;
#endif
    snprintf (s->filename, BUFSIZ, "%s.vtu", basename);
    s->global_num_points = s->points_per_patch * domain->global_num_patches;
    s->global_num_cells = s->cells_per_patch * domain->global_num_patches;
    s->global_num_connectivity = PATCH_CHILDREN * (s->global_num_cells + 1);
    s->fits32 = s->global_num_points <= INT32_MAX
        && s->global_num_connectivity <= INT32_MAX;
    s->inttype = s->fits32 ? "Int32" : "Int64";
    s->intsize = s->fits32 ? sizeof (int32_t) : sizeof (int64_t);
    s->ndsize = 8;   /* uint64 */
    s->coordinate_cb = coordinate_cb;
    s->value_cb = value_cb;

    /* compute data size per patch for the various VTK data arrays */
    s->psize_position = s->points_per_patch * 3 * sizeof (double);
    s->psize_connectivity = s->cells_per_patch * PATCH_CHILDREN * s->intsize;
    s->psize_offsets = s->cells_per_patch * s->intsize;
    s->psize_types = s->cells_per_patch * 1;
    s->psize_mpirank = s->cells_per_patch * 4;
    s->psize_blockno = s->cells_per_patch * 4;
    s->psize_patchno = s->cells_per_patch * s->intsize;
    s->psize_meqn = s->cells_per_patch * s->meqn * sizeof (float);

    /* compute offsets in bytes after beginning of appended data section */
    s->offset_position = 0;
    s->offset_connectivity = s->ndsize +
        s->offset_position + s->psize_position * domain->global_num_patches;
    s->offset_offsets = s->ndsize +
        s->offset_connectivity +
        s->psize_connectivity * domain->global_num_patches;
    s->offset_types = s->ndsize +
        s->offset_offsets + s->psize_offsets * domain->global_num_patches;
    s->offset_mpirank = s->ndsize +
        s->offset_types + s->psize_types * domain->global_num_patches;
    s->offset_blockno = s->ndsize +
        s->offset_mpirank + s->psize_mpirank * domain->global_num_patches;
    s->offset_patchno = s->ndsize +
        s->offset_blockno + s->psize_blockno * domain->global_num_patches;
    s->offset_meqn = s->ndsize +
        s->offset_patchno + s->psize_patchno * domain->global_num_patches;
    s->offset_end = s->ndsize +
        s->offset_meqn + s->psize_meqn * domain->global_num_patches;

    s->buf = NULL;
    s->sink = NULL;
    /* See the documentation of fclaw2d_vtk_state_t for further information. */
    s->patch_threshold = patch_threshold;
    s->num_buffered_patches = 0;

    /* write header meta data and check for error */
    retval = 0;
    if (domain->mpirank == 0)
    {
        retval = fclaw2d_vtk_write_header (glob->domain, s);
    }
    mpiret = sc_MPI_Allreduce (&retval, &gretval, 1, sc_MPI_INT, sc_MPI_MIN,
                               domain->mpicomm);
    SC_CHECK_MPI (mpiret);
    if (gretval < 0)
    {
        return -1;
    }

    /* write mesh and numerical data using MPI I/O */
    fclaw2d_vtk_write_data (glob, s);

    /* write footer information and check for error */
    retval = 0;
    if (domain->mpirank == 0)
    {
        retval = fclaw2d_vtk_write_footer (domain, s);
    }
    mpiret = sc_MPI_Allreduce (&retval, &gretval, 1, sc_MPI_INT, sc_MPI_MIN,
                               domain->mpicomm);
    SC_CHECK_MPI (mpiret);
    if (gretval < 0)
    {
        return -1;
    }

    return 0;
}

static void
fclaw2d_output_vtk_coordinate_cb (fclaw2d_global_t * glob,
                                  fclaw2d_patch_t * patch,
                                  int blockno, int patchno,
                                  char *a)
{
    int mx,my,mbc;
    double dx,dy,xlower,ylower;
#if PATCH_DIM == 2
    fclaw2d_clawpatch_grid_data(glob,patch,&mx,&my,&mbc,
                                &xlower,&ylower,&dx,&dy);

    const fclaw_options_t *fclaw_opt = fclaw2d_get_options(glob);

    /* Enumerate point coordinates in the patch */
    double *d = (double *) a;
    int i, j;
    double xpp,ypp,zpp;
    for (j = 0; j <= my; ++j)
    {
        const double y = ylower + j * dy;
        for (i = 0; i <= mx; ++i)
        {
            const double x = xlower + i * dx;
            if (fclaw_opt->manifold)
            {
                fclaw2d_map_context_t *cont = glob->cont;
                FCLAW2D_MAP_C2M(&cont,&blockno,&x,&y,&xpp,&ypp,&zpp);
                *d++ = xpp;
                *d++ = ypp;
                *d++ = zpp;
            }
            else
            {
                *d++ = x;
                *d++ = y;
                *d++ = 0;
            }
        }
    }
#elif PATCH_DIM == 3
    int mz;
    double zlower, dz;
    fclaw2d_clawpatch_grid_data(glob,patch,&mx,&my,&mz, &mbc,
                                &xlower,&ylower,&zlower, &dx,&dy, &dz);

    const fclaw_options_t *fclaw_opt = fclaw2d_get_options(glob);
    /* Enumerate point coordinates in the patch */
    double *d = (double *) a;
    int i, j, k;
    double xpp,ypp,zpp;
    for (k = 0; k <= mz; ++k)
    {
        const double z = zlower + k * dz;
        for (j = 0; j <= my; ++j)
        {
            const double y = ylower + j * dy;
            for (i = 0; i <= mx; ++i)
            {
                const double x = xlower + i * dx;
                if (fclaw_opt->manifold)
                {
                    fclaw2d_map_context_t *cont = glob->cont;
                    FCLAW3D_MAP_C2M(&cont,&blockno,&x,&y,&z,&xpp,&ypp,&zpp);
                    *d++ = xpp;
                    *d++ = ypp;
                    *d++ = zpp;
                }
                else
                {
                    *d++ = x;
                    *d++ = y;
                    *d++ = z;
                }
            }
        }
    }
#endif
}


static void
fclaw2d_output_vtk_value_cb (fclaw2d_global_t * glob,
                             fclaw2d_patch_t * patch,
                             int blockno, int patchno,
                             char *a)
{

//    fclaw2d_clawpatch_vtable_t *clawpatch_vt = fclaw2d_clawpatch_vt(glob);

    int meqn;
    double *q;
    fclaw2d_clawpatch_soln_data(glob,patch,&q,&meqn);

    int mx,my,mbc;
    double xlower,ylower,dx,dy;
#if PATCH_DIM == 2

    fclaw2d_clawpatch_grid_data(glob,patch,&mx,&my,&mbc,
                                &xlower,&ylower,&dx,&dy);

    const int xlane = mx + 2 * mbc;
    const int ylane = my + 2 * mbc;

    // Enumerate equation data in the patch
    float *f = (float *) a;
    int i, j, k;
    for (j = 0; j < my; ++j)
    {
        for (i = 0; i < mx; ++i)
        {
            for (k = 0; k < meqn; ++k)
            {
                /* For Clawpack 5.0 layout */
                //*f++ = (float) q[((j+mbc)*xlane + (i+mbc))*meqn + k];

                /* For Clawpack 4.x layout */
                *f++ = (float) q[(k * ylane + j + mbc) * xlane + i + mbc];
            }
        }
    }
#elif PATCH_DIM == 3
    int mz;
    double zlower, dz;
    fclaw2d_clawpatch_grid_data(glob,patch,&mx,&my,&mz, &mbc,
                                &xlower,&ylower,&zlower, &dx,&dy, &dz);

    const int xlane = mx + 2 * mbc;
    const int ylane = my + 2 * mbc;
    const int zlane = mz + 2 * mbc;

    // Enumerate equation data in the patch
    float *f = (float *) a;
    int i, j, k, eqn;
    for (k = 0; k < mz; ++k)
    {
        for (j = 0; j < my; ++j)
        {
            for (i = 0; i < mx; ++i)
            {
                for (eqn = 0; eqn < meqn; ++eqn)
                {
                    /* For Clawpack 5.0 layout */
                    //*f++ = (float) q[((j+mbc)*xlane + (i+mbc))*meqn + k];

                    /* For Clawpack 4.x layout */
                    *f++ = (float) q[eqn * zlane * ylane * xlane + (k + mbc) * ylane * xlane + (j + mbc) * xlane + i + mbc];
                }
            }
        }
    }
#endif
}

/*  --------------------------------------------------------------------------
    Used for debugging
    ------------------------------------------------------------------------- */
#if 0
static void
fclaw2d_output_write_vtk_debug (fclaw2d_global_t * glob, const char *basename)
{
    const fclaw_options_t *fclaw_opt = fclaw2d_get_options(glob);
    const fclaw2d_clawpatch_options_t *clawpatch_opt = fclaw2d_clawpatch_get_options(glob);

    (void) fclaw2d_vtk_write_file (glob, basename,
                                   clawpatch_opt->mx, clawpatch_opt->my,
#if PATCH_DIM == 3
                                   clawpatch_opt->mz,
#endif
                                   clawpatch_opt->meqn,
                                   fclaw_opt->vtkspace, 0,
                                   fclaw2d_output_vtk_coordinate_cb,
                                   fclaw2d_output_vtk_value_cb);
}
#endif


/*  ---------------------------------------------------------------------------
    Public interface
    --------------------------------------------------------------------------- */

void fclaw2d_clawpatch_output_vtk (fclaw2d_global_t * glob, int iframe)
{
    const fclaw_options_t *fclaw_opt = fclaw2d_get_options(glob);
    const fclaw2d_clawpatch_options_t *clawpatch_opt = fclaw2d_clawpatch_get_options(glob);


    char basename[BUFSIZ];
    snprintf (basename, BUFSIZ, "%s_frame_%04d", fclaw_opt->prefix, iframe);

    /* The last parameter of the call of fclaw2d_vtk_write_file can be replaced
     * by a value from flcaw_opt or clawpatch_opt.
     */
    (void) fclaw2d_vtk_write_file (glob, basename,
                                   clawpatch_opt->mx, clawpatch_opt->my,
#if PATCH_DIM == 3
                                   clawpatch_opt->mz,
#endif
                                   clawpatch_opt->meqn,
                                   fclaw_opt->vtkspace, 0,
                                   fclaw2d_output_vtk_coordinate_cb,
                                   fclaw2d_output_vtk_value_cb, clawpatch_opt->vtk_patch_threshold);
}


