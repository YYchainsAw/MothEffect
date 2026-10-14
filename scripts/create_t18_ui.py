"""Seed an empty T18 Designer tree; preserve an existing layout and user-edited defaults.

Compile/check only this Widget Blueprint and save only it and a newly created tutorial config.
An existing tree is never regenerated. A missing required control is an error, not a reset.
"""
from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.project_dir())
cache = root / 'Saved/Automation/T18Designer'
cache.mkdir(parents=True, exist_ok=True)
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
config_path = '/Game/MothEffect/UI/DA_MothTutorial'
config_created = not assets.does_asset_exist(config_path)
if config_created:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.MothTutorialConfig)
    config = asset_tools.create_asset('DA_MothTutorial', '/Game/MothEffect/UI', unreal.DataAsset, factory)
else:
    config = unreal.load_asset(config_path)
if not isinstance(config, unreal.MothTutorialConfig):
    raise RuntimeError('Unexpected teaching config type: ' + config_path)
if config_created and not assets.save_loaded_asset(config):
    raise RuntimeError('Cannot save the new teaching config')

path = '/Game/MothEffect/UI/WBP_MothHUD'
if assets.does_asset_exist(path):
    widget = unreal.load_asset(path)
else:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.MothHUDWidget)
    widget = asset_tools.create_asset('WBP_MothHUD', '/Game/MothEffect/UI', unreal.WidgetBlueprint, factory)
report = json.loads(unreal.MothUIEditorLibrary.prepare_designer_layout(widget))
if 'error' in report:
    raise RuntimeError(report['error'])
if not assets.save_loaded_asset(widget):
    raise RuntimeError('Cannot save the checked Widget Blueprint')
cls = unreal.load_class(None, path + '.WBP_MothHUD_C')
if not cls or not isinstance(unreal.get_default_object(cls), unreal.MothHUDWidget):
    raise RuntimeError('Saved Widget Blueprint has the wrong native parent')
report.update(package=path, config_package=config_path, config_created=config_created, saved=True)
name = 'AssetVerificationCreated.json' if report['created_layout'] else 'AssetVerificationReloaded.json'
(cache / name).write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('T18_DESIGNER_ASSET_VERIFIED ' + json.dumps(report))
unreal.SystemLibrary.quit_editor()
