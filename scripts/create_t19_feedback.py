"""Create missing feedback assets only; preserve all existing user-edited packages."""
from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.project_dir())
cache = root / 'Saved/Automation/T19'
cache.mkdir(parents=True, exist_ok=True)
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/MothEffect/Feedback'
created = []


def create(name, cls, factory, package=folder):
    path = package + '/' + name
    if assets.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError('Unexpected existing asset type: ' + path)
        return asset, False
    asset = tools.create_asset(name, package, cls, factory)
    if not asset:
        raise RuntimeError('Cannot create ' + path)
    created.append(path)
    return asset, True


material, new = create('M_DefaultFeedback', unreal.Material, unreal.MaterialFactoryNew())
if new:
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, 0)
    color.set_editor_property('parameter_name', 'Color')
    color.set_editor_property('default_value', unreal.LinearColor(1, 1, 1, 1))
    intensity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 180)
    intensity.set_editor_property('parameter_name', 'Intensity')
    intensity.set_editor_property('default_value', 2.)
    multiply = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, 0)
    unreal.MaterialEditingLibrary.connect_material_expressions(color, '', multiply, 'A')
    unreal.MaterialEditingLibrary.connect_material_expressions(intensity, '', multiply, 'B')
    unreal.MaterialEditingLibrary.connect_material_property(multiply, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not assets.save_loaded_asset(material):
        raise RuntimeError('Cannot save feedback material')

attenuation, new = create('SA_DefaultFeedback', unreal.SoundAttenuation, unreal.SoundAttenuationFactory())
if new:
    settings = attenuation.get_editor_property('attenuation')
    settings.set_editor_property('attenuate', True)
    settings.set_editor_property('spatialize', True)
    settings.set_editor_property('attenuation_shape_extents', unreal.Vector(250, 0, 0))
    settings.set_editor_property('falloff_distance', 2250.)
    attenuation.set_editor_property('attenuation', settings)
    if not assets.save_loaded_asset(attenuation):
        raise RuntimeError('Cannot save attenuation')

manifest = json.loads((root / 'SourceArt/Feedback/AudioManifest.json').read_text(encoding='utf-8'))
for entry in manifest['sounds']:
    name = entry['name']
    package = folder + '/Audio/' + name
    if not assets.does_asset_exist(package):
        task = unreal.AssetImportTask()
        task.filename = str(root / 'SourceArt/Feedback' / (name + '.wav'))
        task.destination_path = folder + '/Audio'
        task.destination_name = name
        task.automated = True
        task.save = True
        task.replace_existing = False
        tools.import_asset_tasks([task])
        if package not in task.imported_object_paths and not assets.does_asset_exist(package):
            raise RuntimeError('Cannot import ' + package)
        created.append(package)
    if not isinstance(unreal.load_asset(package), unreal.SoundWave):
        raise RuntimeError('Invalid sound wave: ' + package)

factory = unreal.DataAssetFactory()
factory.set_editor_property('data_asset_class', unreal.MothFeedbackProfile)
profile, new = create('DA_DefaultFeedback', unreal.MothFeedbackProfile, factory)
if new and not assets.save_loaded_asset(profile):
    raise RuntimeError('Cannot save feedback profile')
styles = profile.get_editor_property('styles')
if len(styles) != 12:
    raise RuntimeError('Expected twelve cue slots')
report = dict(created=created, profile=profile.get_path_name(), cue_slots=len(styles),
              sounds=len(manifest['sounds']), material=material.get_path_name(),
              attenuation=attenuation.get_path_name(), existing_assets_preserved=True)
(cache / ('AssetsCreated.json' if created else 'AssetsReloaded.json')).write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('T19_FEEDBACK_ASSETS ' + json.dumps(report))
unreal.SystemLibrary.quit_editor()
