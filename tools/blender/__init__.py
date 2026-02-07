import bpy
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator
import struct
import array

#class crude_vertex:
#  XMFLOAT3A                                                position;
#  uint8                                                    normal[ 4 ];
#  uint8                                                    tangent[ 4 ];
#  uint16                                                   texcoords[ 2 ];

def crude_export_mesh_( mesh, vertices, indices ):
    ###################################################################################
    # bpy.types.Mesh                                       mesh 
    # crude_vertex                                         vertices
    # int                                                  indices
    # int                                                  indices_offest
    # int                                                  vertices_offest
    # int                                                  mesh_triangles_count
    # int                                                  mesh_vertices_count
    ###################################################################################
    mesh_triangles_count = len( mesh.loop_triangles )
    mesh_vertices_count = len( mesh.vertices )

    vertices_offest = len( vertices )
    indices_offest = len( indices )

    indices.extend( [ 0 ] * 3 * mesh_triangles_count )
    vertices.extend( [ 0 ] * mesh_vertices_count )

    for triangle_index in range( mesh_triangles_count ):
        indices[ indices_offest + 3 * triangle_index + 0 ] = mesh.loop_triangles[ triangle_index ].loops[ 0 ]
        indices[ indices_offest + 3 * triangle_index + 1 ] = mesh.loop_triangles[ triangle_index ].loops[ 1 ]
        indices[ indices_offest + 3 * triangle_index + 2 ] = mesh.loop_triangles[ triangle_index ].loops[ 2 ]
    
    for vertex_index in range( mesh_vertices_count ):
        vertices[ vertex_index ].tangent = mesh.vertices[ vertex_index ].tangent
        vertices[ vertex_index ].normal = mesh.vertices[ vertex_index ].normal
        vertices[ vertex_index ].position = mesh.vertices[ vertex_index ].position

def crude_export_object_( object, vertices, indices ):
    ###################################################################################
    # bpy.types.Object                                     object 
    # crude_vertex                                         vertices
    # int                                                  indices
    ###################################################################################
    if object.type == 'MESH':
        object.calc_tangents( )
        crude_export_mesh_( object.data, vertices, indices )
    
    for child_object in object.children:
        crude_export_object_( child_object, vertices, indices )

def crude_export_( context,filepath, export_selected ):
    ###################################################################################
    # int                                                  indices
    # crude_vertex                                         vertices
    # bytearray                                            vertices_buffer
    # array.array                                          indices_buffer
    ###################################################################################
    
    indices = []
    vertices = []

    f = open( filepath, "wb" )
    for object in context.scene.objects:
        crude_export_object_( object, vertices, indices )
    
    # Write vertices
    vertices_buffer = bytearray()
    for vertex in vertices:
        # Position: 3 floats (12 bytes)
        vertices_buffer.extend( struct.pack( '>fff', vertex[ 'position' ].x, vertex[ 'position' ].y, vertex[ 'position' ].z ) )
            
        # Normal: 4 uint8 (4 bytes)
        x = int( ( vertex[ 'normal' ].x * 0.5 + 0.5 ) * 255 )
        y = int( ( vertex[ 'normal' ].y * 0.5 + 0.5 ) * 255 )
        z = int( ( vertex[ 'normal' ].z * 0.5 + 0.5 ) * 255 )
        vertices_buffer.extend( struct.pack( '>BBBB', x, y, z, 0 ) )
            
        # Tangent: 4 uint8 (4 bytes)
        x = int( ( vertex[ 'tangent' ].x * 0.5 + 0.5 ) * 255 )
        y = int( ( vertex[ 'tangent' ].y * 0.5 + 0.5 ) * 255 )
        z = int( ( vertex[ 'tangent' ].z * 0.5 + 0.5 ) * 255 )
        vertices_buffer.extend( struct.pack( '>BBBB', x, y, z, 0 ) )
            
        # Texcoords: 2 uint16 (4 bytes)
        u = int( vertex[ 'texcoords' ][ 0 ] * 65535 )
        v = int( vertex[ 'texcoords' ][ 1 ] * 65535 )
        vertices_buffer.extend( struct.pack( '>HH', u, v ) )
    
    f.write(vertices_buffer)

    # Write indices
    indices_buffer = array.array( 'I', indices )
    indices_buffer.tofile( f )
    
    f.close()

    return {'FINISHED'}


class ExportCrude(Operator, ExportHelper):
    bl_idname = "data.crudeb"
    bl_label = "Export Crude"

    filename_ext = ".crudeb"

    filter_glob: StringProperty(
        default="*.crudeb",
        options={'HIDDEN'},
        maxlen=255,
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    export_selected: BoolProperty(
        name="Export Selected",
        description="",
        default=False,
    )

    type: EnumProperty(
        name="Example Enum",
        description="Choose between two items",
        items=(
            ('OPT_A', "First Option", "Description one"),
            ('OPT_B', "Second Option", "Description two"),
        ),
        default='OPT_A',
    )

    def execute(self, context):
        return crude_export_( context, self.filepath, self.export_selected )


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportCrude.bl_idname, text="Export Crude")


# Register and add to the "file selector" menu (required to use F3 search "Text Export Operator" for quick access).
def register():
    bpy.utils.register_class(ExportCrude)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
