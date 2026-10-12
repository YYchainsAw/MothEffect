"""Create/reload only the T18-owned Widget Blueprint. Never save existing player assets."""
from pathlib import Path
import json
import unreal

root = Path(unreal.Paths.project_dir())
cache = root / 'Saved/Automation/T18'
cache.mkdir(parents=True, exist_ok=True)
marker = cache / 'CreatedPackages.json'
owned = json.loads(marker.read_text(encoding='utf-8')) if marker.exists() else []
path = '/Game/MothEffect/UI/WBP_MothHUD'
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
if assets.does_asset_exist(path) and path not in owned:
    raise RuntimeError('Refuse to replace a non-T18-owned package: ' + path)
if path not in owned:
    owned.append(path)
    marker.write_text(json.dumps(owned, indent=2), encoding='utf-8')
if assets.does_asset_exist(path):
    widget = unreal.load_asset(path)
else:
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property('parent_class', unreal.MothHUDWidget)
    widget = unreal.AssetToolsHelpers.get_asset_tools().create_asset('WBP_MothHUD', '/Game/MothEffect/UI', unreal.WidgetBlueprint, factory)
if not widget or not assets.save_loaded_asset(widget):
    raise RuntimeError('Cannot create/save the owned Widget Blueprint')
cls = unreal.load_class(None, path + '.WBP_MothHUD_C')
if not cls or not isinstance(unreal.get_default_object(cls), unreal.MothHUDWidget):
    raise RuntimeError('Saved Widget Blueprint has the wrong native parent')
(cache / 'AssetVerification.json').write_text(json.dumps({'package': path, 'class': cls.get_path_name(), 'native_parent': '/Script/MothEffect.MothHUDWidget', 'saved': True}, indent=2), encoding='utf-8')
unreal.log('T18_UI_ASSET_VERIFIED ' + cls.get_path_name())
unreal.SystemLibrary.quit_editor()
