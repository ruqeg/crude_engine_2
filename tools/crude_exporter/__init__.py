import bpy


def crude_export(context,filepath,export_selected):
    print("running crude_export...")
    f = open(filepath, "w", encoding='utf-8')
    for object in context.scene.objects:
        f.write(object.name)
    f.close()

    return {'FINISHED'}


from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


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
        return crude_export(context, self.filepath, self.export_selected)


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
