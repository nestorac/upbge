/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BLI_array_utils.hh"

#include "DNA_mesh_types.h"

#include "node_geometry_util.hh"

namespace blender::nodes::node_geo_input_mesh_vertex_neighbors_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_output<decl::Int>("Vertex Count")
      .field_source()
      .description(
          "The number of vertices connected to this vertex with an edge, "
          "equal to the number of connected edges");
  b.add_output<decl::Int>("Face Count")
      .field_source()
      .description("Number of faces that contain the vertex");
}

class VertexCountFieldInput final : public bke::MeshFieldInput {
 public:
  VertexCountFieldInput() : bke::MeshFieldInput(CPPType::get<int>(), "Vertex Count Field")
  {
    category_ = Category::Generated;
  }

  GVArray get_varray_for_context(const Mesh &mesh,
                                 const AttrDomain domain,
                                 const IndexMask & /*mask*/) const final
  {
    if (domain != AttrDomain::Point) {
      return {};
    }
    Array<int> counts(mesh.verts_num, 0);
    array_utils::count_indices(mesh.edges().cast<int>(), counts);
    return VArray<int>::from_container(std::move(counts));
  }

  uint64_t hash() const override
  {
    /* Some random constant hash. */
    return 23574528465;
  }

  bool is_equal_to(const fn::FieldNode &other) const override
  {
    return dynamic_cast<const VertexCountFieldInput *>(&other) != nullptr;
  }

  std::optional<AttrDomain> preferred_domain(const Mesh & /*mesh*/) const override
  {
    return AttrDomain::Point;
  }
};

class VertexFaceCountFieldInput final : public bke::MeshFieldInput {
 public:
  VertexFaceCountFieldInput() : bke::MeshFieldInput(CPPType::get<int>(), "Vertex Face Count Field")
  {
    category_ = Category::Generated;
  }

  GVArray get_varray_for_context(const Mesh &mesh,
                                 const AttrDomain domain,
                                 const IndexMask & /*mask*/) const final
  {
    if (domain != AttrDomain::Point) {
      return {};
    }
    Array<int> counts(mesh.verts_num, 0);
    array_utils::count_indices(mesh.corner_verts(), counts);
    return VArray<int>::from_container(std::move(counts));
  }

  uint64_t hash() const override
  {
    /* Some random constant hash. */
    return 3462374322;
  }

  bool is_equal_to(const fn::FieldNode &other) const override
  {
    return dynamic_cast<const VertexFaceCountFieldInput *>(&other) != nullptr;
  }

  std::optional<AttrDomain> preferred_domain(const Mesh & /*mesh*/) const override
  {
    return AttrDomain::Point;
  }
};

static void node_geo_exec(GeoNodeExecParams params)
{
  Field<int> vertex_field{std::make_shared<VertexCountFieldInput>()};
  Field<int> face_field{std::make_shared<VertexFaceCountFieldInput>()};

  params.set_output("Vertex Count", std::move(vertex_field));
  params.set_output("Face Count", std::move(face_field));
}

static void node_register()
{
  static blender::bke::bNodeType ntype;
  geo_node_type_base(
      &ntype, "GeometryNodeInputMeshVertexNeighbors", GEO_NODE_INPUT_MESH_VERTEX_NEIGHBORS);
  ntype.ui_name = "Vertex Neighbors";
  ntype.ui_description = "Retrieve topology information relating to each vertex of a mesh";
  ntype.enum_name_legacy = "MESH_VERTEX_NEIGHBORS";
  ntype.nclass = NODE_CLASS_INPUT;
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  blender::bke::node_register_type(ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_input_mesh_vertex_neighbors_cc
