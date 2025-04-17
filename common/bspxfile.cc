/*  Copyright (C) 1996-1997  Id Software, Inc.

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    See file, 'COPYING', for details.
*/

#include <common/bspxfile.hh>

#include <common/cmdlib.hh>

// bspx_header_t

bspx_header_t::bspx_header_t(uint32_t numlumps)
    : numlumps(numlumps)
{
}

void bspx_header_t::stream_write(std::ostream &s) const
{
    s <= std::tie(id, numlumps);
}

void bspx_header_t::stream_read(std::istream &s)
{
    s >= std::tie(id, numlumps);
}

// bspx_lump_t

void bspx_lump_t::stream_write(std::ostream &s) const
{
    s <= std::tie(lumpname, fileofs, filelen);
}

void bspx_lump_t::stream_read(std::istream &s)
{
    s >= std::tie(lumpname, fileofs, filelen);
}

// bspxbrushes_perbrush

void bspxbrushes_perbrush::stream_write(std::ostream &s) const
{
    s <= bounds;
    s <= contents;
    s <= static_cast<uint16_t>(faces.size());

    for (auto &face : faces) {
        s <= face;
    }
}

void bspxbrushes_perbrush::stream_read(std::istream &s)
{
    s >= bounds;
    s >= contents;

    uint16_t numfaces = 0;
    s >= numfaces;

    faces.resize(numfaces);

    for (auto &face : faces) {
        s >= face;
    }
}

// bspxbrushes_permodel

void bspxbrushes_permodel::stream_write(std::ostream &s) const
{
    s <= ver;
    s <= modelnum;
    s <= static_cast<int32_t>(brushes.size());
    // count faces (ignore numfaces)
    int32_t faces = 0;
    for (auto &brush : brushes) {
        faces += static_cast<int32_t>(brush.faces.size());
    }
    s <= faces;

    // next serialize all of the brushes
    for (auto &brush : brushes) {
        s <= brush;
    }
}

void bspxbrushes_permodel::stream_read(std::istream &s)
{
    s >= ver;
    if (!s) {
        // we need to handle end-of-stream due to the bspx lump containing an unknown number
        // of bspxbrushes_permodel objects
        return;
    }

    s >= modelnum;

    int32_t numbrushes;
    s >= numbrushes;
    s >= numfaces;

    brushes.resize(numbrushes);
    for (auto &brush : brushes) {
        s >= brush;
    }
}

// bspxbrushes

void bspxbrushes::stream_write(std::ostream &s) const
{
    for (auto &model : models) {
        s <= model;
    }
}

void bspxbrushes::stream_read(std::istream &s)
{
    models.clear();

    while (true) {
        bspxbrushes_permodel model;
        s >= model;

        if (!s) {
            break;
        }

        models.push_back(std::move(model));
    }
}

// bspxfacenormals_per_vert

void bspxfacenormals_per_vert::stream_write(std::ostream &s) const
{
    s <= std::tie(normal, tangent, bitangent);
}

void bspxfacenormals_per_vert::stream_read(std::istream &s)
{
    s >= std::tie(normal, tangent, bitangent);
}

// bspxfacenormals_per_face

void bspxfacenormals_per_face::stream_write(std::ostream &s) const
{
    for (const auto &v : per_vert) {
        s <= v;
    }
}

void bspxfacenormals_per_face::stream_read(std::istream &s, const mface_t &f)
{
    for (int i = 0; i < f.numedges; ++i) {
        bspxfacenormals_per_vert v;
        s >= v;
        per_vert.push_back(v);
    }
}

// bspxfacenormals

void bspxfacenormals::stream_write(std::ostream &s) const
{
    // write the table of normals
    s <= static_cast<uint32_t>(normals.size());

    for (const qvec3f &v : normals) {
        s <= v;
    }

    // write the per-face, per-vertex indices into the prior table
    for (const auto &f : per_face) {
        s <= f;
    }
}

void bspxfacenormals::stream_read(std::istream &s, const mbsp_t &bsp)
{
    normals.clear();
    per_face.clear();

    // read normals table
    uint32_t size;
    s >= size;

    for (uint32_t i = 0; i < size; ++i) {
        qvec3f v;
        s >= v;
        normals.push_back(v);
    }

    // read, based on the faces in the provided bsp
    for (const auto &f : bsp.dfaces) {
        bspxfacenormals_per_face pf;
        pf.stream_read(s, f);
        per_face.push_back(pf);
    }
}

// bspx_decoupled_lm_perface

void bspx_decoupled_lm_perface::stream_write(std::ostream &s) const
{
    s <= std::tie(lmwidth, lmheight, offset, world_to_lm_space);
}

void bspx_decoupled_lm_perface::stream_read(std::istream &s)
{
    s >= std::tie(lmwidth, lmheight, offset, world_to_lm_space);
}

// bspx_leaf_sprop

void bspx_leaf_sprop::stream_write(std::ostream &s) const
{
    // write entries
    s <= static_cast<uint32_t>(entries.size());
    s <= static_cast<uint32_t>(prop_indices.size());

    for (const auto &entry : entries) {
        s <= entry.first_prop;
        s <= entry.num_props;
    }

    for (auto entry : prop_indices) {
        s <= entry;
    }
}

void bspx_leaf_sprop::stream_read(std::istream &s)
{
    entries.clear();
    prop_indices.clear();

    uint32_t num_entries;
    s >= num_entries;

    uint32_t num_indices;
    s >= num_indices;

    for (uint32_t i = 0; i < num_entries; i++) {
        bspx_leaf_sprop_entry entry;
        s >= entry.first_prop;
        s >= entry.num_props;

        entries.push_back(entry);
    }

    for (uint32_t i = 0; i < num_indices; i++) {
        uint32_t prop_index;
        s >= prop_index;

        prop_indices.push_back(prop_index);
    }
}

// bspx_sprop

void bspx_sprop::stream_write(std::ostream &s) const
{
    s <= static_cast<uint32_t>(entries.size());

    for (const auto &entry : entries) {
        s <= entry.material;
        s <= entry.mode;
        s <= entry.first_index;
        s <= entry.num_indices;
        s <= entry.first_vertex;
        s <= entry.num_vertices;
    }
}

void bspx_sprop::stream_read(std::istream &s)
{
    entries.clear();

    uint32_t num_entries;
    s >= num_entries;

    for (uint32_t i = 0; i < num_entries; i++) {
        bspx_sprop_entry entry;
        s >= entry.material;
        s >= entry.mode;
        s >= entry.first_index;
        s >= entry.num_indices;
        s >= entry.first_vertex;
        s >= entry.num_vertices;

        entries.push_back(entry);
    }
}

// bspx_sprop_indices

void bspx_sprop_indices::stream_write(std::ostream &s) const
{
    s <= static_cast<uint32_t>(indices.size());

    for (auto idx : indices) {
        s <= idx;
    }
}

void bspx_sprop_indices::stream_read(std::istream &s)
{
    indices.clear();

    uint32_t num_entries;
    s >= num_entries;

    for (uint32_t i = 0; i < num_entries; i++) {
        uint16_t idx;
        s >= idx;

        indices.push_back(idx);
    }
}

// bspx_sprop_vertices

void bspx_sprop_vertices::stream_write(std::ostream &s) const
{
    s <= static_cast<uint32_t>(vertices.size());

    for (const auto &vertex : vertices) {
        s <= vertex.position;
        s <= vertex.normal;
        s <= vertex.tangent;
        s <= vertex.texcoord;
        s <= vertex.color[0];
        s <= vertex.color[1];
        s <= vertex.color[2];
        s <= vertex.color[3];

        for (int i = 0; i < 4; i++) {
            s <= vertex.light_styles[i];
        }

        for (int i = 0; i < 4; i++) {
            if (vertex.light_styles[i] != 255) {
                s <= vertex.light_colors[i][0];
                s <= vertex.light_colors[i][1];
                s <= vertex.light_colors[i][2];
            }
        }
    }
}

void bspx_sprop_vertices::stream_read(std::istream &s)
{
    vertices.clear();

    uint32_t num_entries;
    s >= num_entries;

    for (uint32_t i = 0; i < num_entries; i++) {
        bspx_sprop_vertex vertex;
        s >= vertex.position;
        s >= vertex.normal;
        s >= vertex.tangent;
        s >= vertex.texcoord;
        s >= vertex.color[0];
        s >= vertex.color[1];
        s >= vertex.color[2];
        s >= vertex.color[3];

        for (int i = 0; i < 4; i++) {
            s >= vertex.light_styles[i];
        }

        for (int i = 0; i < 4; i++) {
            if (vertex.light_styles[i] != 255) {
                s >= vertex.light_colors[i][0];
                s >= vertex.light_colors[i][1];
                s >= vertex.light_colors[i][2];
            }
            else {
                vertex.light_colors[i][0] = 0;
                vertex.light_colors[i][1] = 0;
                vertex.light_colors[i][2] = 0;
            }
        }

        vertices.push_back(vertex);
    }
}

// bspx_sprop_materials

void bspx_sprop_materials::stream_write(std::ostream &s) const
{
    s <= static_cast<uint32_t>(materials.size());

    for (const auto &mat : materials) {
        s <= mat;
    }
}

void bspx_sprop_materials::stream_read(std::istream &s)
{
    materials.clear();

    uint32_t num_entries;
    s >= num_entries;

    for (uint32_t i = 0; i < num_entries; i++) {
        std::array<char, 64> mat;
        s >= mat;

        materials.push_back(mat);
    }
}