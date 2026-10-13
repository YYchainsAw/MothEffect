"""Create and re-extract a complete T21 package, excluding runtime Saved data."""
from argparse import ArgumentParser
from pathlib import Path
import hashlib
import json
import zipfile


def digest(path):
    result=hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda:source.read(1024*1024),b''): result.update(chunk)
    return result.hexdigest()


def main():
    parser=ArgumentParser(description=__doc__)
    parser.add_argument('windows_directory',type=Path)
    parser.add_argument('output_zip',type=Path)
    parser.add_argument('extract_directory',type=Path)
    parser.add_argument('--build-inputs',type=Path,required=True)
    args=parser.parse_args()
    source=args.windows_directory.resolve()
    output=args.output_zip.resolve()
    destination=args.extract_directory.resolve()
    if output.exists() or destination.exists(): raise SystemExit('Use a new ZIP/extraction destination; existing files are preserved.')
    if not (source/'MothEffect.exe').is_file(): raise SystemExit('Complete Windows directory must include the launcher.')
    files=[]
    for path in sorted(source.rglob('*')):
        if not path.is_file(): continue
        relative=path.relative_to(source)
        if relative.parts[:2]==('MothEffect','Saved'): continue
        files.append((path,relative))
    output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=1,allowZip64=True) as archive:
        for path,relative in files: archive.write(path,'Windows/'+relative.as_posix())
    print(f'Archived {len(files)} payload files: {output}',flush=True)
    destination.mkdir(parents=True)
    with zipfile.ZipFile(output) as archive:
        assert archive.testzip() is None
        archive.extractall(destination)
    checks=[]
    for path,relative in files:
        original=digest(path); extracted=destination/'Windows'/relative
        if digest(extracted)!=original: raise RuntimeError('Extracted file hash differs: '+str(relative))
        checks.append(dict(path='Windows/'+relative.as_posix(),bytes=path.stat().st_size,sha256=original))
    inputs=json.loads(args.build_inputs.read_text(encoding='utf-8-sig'))
    manifest=dict(task='T21',configuration=inputs['configuration'],code_commit=inputs['git_commit'],
        input_git_status=inputs['git_status'],user_blueprint_sha256=next(x['sha256'] for x in inputs['input_hashes'] if x['path'].endswith('BP_MothPlayer.uasset')),
        zip_file=output.name,zip_bytes=output.stat().st_size,zip_sha256=digest(output),payload_files=checks,
        extracted_hashes_match=True,launcher='Windows/MothEffect.exe',runtime_saved_excluded=True,
        user_acceptance='Pending; internal automation does not close manual T21/G3.')
    manifest_path=output.with_suffix('.manifest.json')
    manifest_path.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'ZIP integrity and {len(checks)} extracted SHA256 hashes verified. Manifest: {manifest_path}',flush=True)


if __name__=='__main__': main()
