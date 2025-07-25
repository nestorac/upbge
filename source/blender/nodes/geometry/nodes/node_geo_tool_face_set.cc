/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BKE_geometry_fields.hh"

#include "node_geometry_util.hh"

namespace blender::nodes::node_geo_tool_face_set_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_output<decl::Int>("Face Set").field_source();
  b.add_output<decl::Bool>("Exists").field_source();
}

static void node_geo_exec(GeoNodeExecParams params)
{
  if (!check_tool_context_and_error(params)) {
    return;
  }
  params.set_output("Face Set", bke::AttributeFieldInput::from<int>(".sculpt_face_set"));
  params.set_output("Exists", bke::AttributeExistsFieldInput::from(".sculpt_face_set"));
}

static void node_register()
{
  static blender::bke::bNodeType ntype;
  geo_node_type_base(&ntype, "GeometryNodeToolFaceSet", GEO_NODE_TOOL_FACE_SET);
  ntype.ui_name = "Face Set";
  ntype.ui_description = "Each face's sculpt face set value";
  ntype.enum_name_legacy = "TOOL_FACE_SET";
  ntype.nclass = NODE_CLASS_INPUT;
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  ntype.gather_link_search_ops = search_link_ops_for_tool_node;
  blender::bke::node_register_type(ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_tool_face_set_cc
