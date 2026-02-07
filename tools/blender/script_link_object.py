import pathlib
import bpy
from bpy_extras.object_utils import AddObjectHelper
from bpy.props import (FloatProperty,StringProperty)

def remove_libraries():
    bpy.data.batch_remove(bpy.data.libraries)


def link_blend_file_objects(blend_file_path, link=False):
    with bpy.data.libraries.load(blend_file_path, link=link) as (data_from, data_to):
        data_to.objects = data_from.objects

    scene = bpy.context.scene

    for obj in data_to.objects:
        if obj is None:
            continue

    scene.collection.objects.link(obj)


def link_blend_file_scenes(blend_file_path, link=False):
    with bpy.data.libraries.load(blend_file_path, link=link) as (data_from, data_to):
        data_to.scenes = data_from.scenes


def link_blend_file_materials(blend_file_path, link=False):
    with bpy.data.libraries.load(blend_file_path, link=link) as (data_from, data_to):

        for material_name in data_from.materials:
            if "blue" in material_name:
                data_to.materials.append(material_name)


class LinkBlendFileObject(bpy.types.Operator, AddObjectHelper):
    bl_idname = "mesh.primitive_box_add"
    bl_label = "Link Blend File Object"
    bl_options = {'REGISTER', 'UNDO'}

    filename_ext = ".blend"

    filter_glob: StringProperty(
        default="*.txt",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        remove_libraries()
        blend_file_path = str(pathlib.Path().home() / "tmp" / "objects.blend")
        link_blend_file_objects(blend_file_path, link=True)
        return {'FINISHED'}

def menu_func(self, context):
    self.layout.operator(LinkBlendFileObject.bl_idname, icon='MESH')


def register():
    bpy.utils.register_class(LinkBlendFileObject)
    bpy.types.VIEW3D_MT_mesh_add.append(menu_func)


def unregister():
    bpy.utils.unregister_class(LinkBlendFileObject)
    bpy.types.VIEW3D_MT_mesh_add.remove(menu_func)


if __name__ == "__main__":
    register()
