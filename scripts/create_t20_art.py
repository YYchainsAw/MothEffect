"""Import missing T20 assets in UE Editor Python; preserve all existing packages.

Run after compiling MothEffectEditor. No SaveAll, Blueprint edits or map resaves.
Source OBJ units are centimeters, Z up, +X forward. Legacy FBX imports right-
handed OBJ into Unreal by mirroring Y; validate this conversion explicitly.
"""
from pathlib import Path
import hashlib
import json
import unreal

root = Path(unreal.Paths.project_dir())
cache = root / 'Saved/Automation/T20'
cache.mkdir(parents=True, exist_ok=True)
folder = '/Game/MothEffect/PrototypeArt'
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
mesh_tools = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
created = []
manifest = json.loads((root / 'SourceArt/PrototypeArt/PrototypeArtManifest.json').read_text(encoding='utf-8'))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, 'Interchange.FeatureFlags.Import.OBJ 0')


def create(name, directory, cls, factory):
    path = folder + '/' + directory + '/' + name if directory else folder + '/' + name
    if assets.does_asset_exist(path):
        obj = unreal.load_asset(path)
        if not isinstance(obj, cls):
            raise RuntimeError('Unexpected existing asset type: ' + path)
        return obj, False
    package, leaf = path.rsplit('/', 1)
    obj = tools.create_asset(leaf, package, cls, factory)
    if not obj:
        raise RuntimeError('Cannot create ' + path)
    created.append(path)
    return obj, True


def save(obj):
    if not assets.save_loaded_asset(obj):
        raise RuntimeError('Cannot save new asset ' + obj.get_path_name())


masters = {}
for name, glow in [('M_PrototypeMetal', False), ('M_PrototypeGlow', True)]:
    material, new = create(name, 'Materials', unreal.Material, unreal.MaterialFactoryNew())
    masters[glow] = material
    if not new:
        continue
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property('parameter_name', 'Tint')
    color.set_editor_property('default_value', unreal.LinearColor(.1, .15, .18, 1))
    if glow:
        material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
        strength = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, 150)
        strength.set_editor_property('parameter_name', 'Intensity')
        strength.set_editor_property('default_value', 2.)
        multiply = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -100, 0)
        unreal.MaterialEditingLibrary.connect_material_expressions(color, '', multiply, 'A')
        unreal.MaterialEditingLibrary.connect_material_expressions(strength, '', multiply, 'B')
        unreal.MaterialEditingLibrary.connect_material_property(multiply, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        unreal.MaterialEditingLibrary.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
        for parameter, value, target, y in [('Metallic', .75, unreal.MaterialProperty.MP_METALLIC, 150), ('Roughness', .45, unreal.MaterialProperty.MP_ROUGHNESS, 300)]:
            node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -400, y)
            node.set_editor_property('parameter_name', parameter)
            node.set_editor_property('default_value', value)
            unreal.MaterialEditingLibrary.connect_material_property(node, '', target)
    unreal.MaterialEditingLibrary.recompile_material(material)
    save(material)

materials = {}
for name, rgb, glow in [
    ('MI_Metal', [.12,.15,.17], False), ('MI_Copper', [.55,.25,.08], False),
    ('MI_D01Metal', [.08,.15,.17], False), ('MI_D02Metal', [.18,.12,.08], False), ('MI_D03Metal', [.14,.09,.18], False),
    ('MI_D01Glow', [0,.8,1], True), ('MI_D02Glow', [1,.22,.01], True), ('MI_D03Glow', [.5,.03,1], True),
    ('MI_RuneGlow', [.15,.75,1], True),
]:
    obj, new = create(name, 'Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    materials[name] = obj
    if new:
        unreal.MaterialEditingLibrary.set_material_instance_parent(obj, masters[glow])
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(obj, 'Tint', unreal.LinearColor(*rgb, 1))
        save(obj)

checks = []
for item in manifest['meshes']:
    name = item['name']
    path = folder + '/Meshes/' + name
    source = root / 'SourceArt/PrototypeArt' / item['source_file']
    if hashlib.sha256(source.read_bytes()).hexdigest() != item['source_sha256']:
        raise RuntimeError('Source SHA mismatch; regenerate manifest or keep user asset separate: ' + name)
    new = not assets.does_asset_exist(path)
    if new:
        options = unreal.FbxImportUI()
        options.set_editor_property('automated_import_should_detect_type', False)
        options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_materials', False)
        options.set_editor_property('import_textures', False)
        settings = options.get_editor_property('static_mesh_import_data')
        for key, value in [('combine_meshes', True), ('auto_generate_collision', False), ('convert_scene', False), ('convert_scene_unit', False), ('import_uniform_scale', 1.), ('import_rotation', unreal.Rotator(0,0,0))]:
            settings.set_editor_property(key, value)
        settings.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = folder + '/Meshes'
        task.destination_name = name
        task.automated = True
        task.save = False
        task.replace_existing = False
        task.factory = unreal.FbxFactory()
        task.options = options
        tools.import_asset_tasks([task])
    mesh = unreal.load_asset(path)
    if not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError('Cannot load imported mesh ' + path)
    slots = mesh.get_editor_property('static_materials')
    names = [str(slot.get_editor_property('material_slot_name')) for slot in slots]
    if sorted(names) != sorted(manifest['material_slots']):
        raise RuntimeError('Three named Metal/Copper/Glow slots required: ' + name + ' ' + str(names))
    box = mesh.get_bounding_box()
    actual = dict(min=[box.min.x, box.min.y, box.min.z], max=[box.max.x, box.max.y, box.max.z])
    if new:
        for edge in ('min', 'max'):
            expected = list(item['bounds_cm'][edge])
            expected[1] = -item['bounds_cm']['max' if edge == 'min' else 'min'][1]
            if any(abs(a-b) > .05 for a,b in zip(actual[edge], expected)):
                raise RuntimeError('OBJ import changed centimeters or axes: ' + name + ' ' + str(actual))
        for index, role in enumerate(names):
            mesh.set_material(index, materials[{'Metal':'MI_Metal', 'Copper':'MI_Copper', 'Glow':'MI_RuneGlow'}[role]])
        save(mesh)
        created.append(path)
    checks.append(dict(name=name, bounds_cm=actual, slots=names, simple_collision_count=mesh_tools.get_simple_collision_count(mesh), new=new))

factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.MothPrototypeArtProfile)
profile, new = create('DA_PrototypeArt', '', unreal.MothPrototypeArtProfile, factory)
if new:
    save(profile)
if len(profile.get_editor_property('devices')) != 3:
    raise RuntimeError('Expected three device appearances')
report = dict(created=created, mesh_checks=checks, profile=profile.get_path_name(), device_slots=3,
              existing_assets_preserved=True, blueprints_and_maps_saved=False)
(cache / ('AssetsCreated.json' if created else 'AssetsReloaded.json')).write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('T20_ART_ASSETS ' + json.dumps(report))
unreal.SystemLibrary.quit_editor()
