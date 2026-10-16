"""Build the existing Docs reading view with gh's GitHub Markdown renderer.

Uses Python's standard library and an authenticated gh CLI. Unchanged sections
are reused when their source hashes match. --check validates without networking.
"""
import argparse
import hashlib
import html
from html.parser import HTMLParser
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
from urllib.parse import unquote, urlsplit
import json

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / 'Docs/01_Project_Overview/Reading_View.html'
DOCS = [
    ('doc-readme', '文档入口', 'Docs/README.md'),
    ('doc-overview', '项目概览', 'Docs/01_Project_Overview/Project_Overview.md'),
    ('doc-game', '游戏策划案', 'Docs/02_Design_Doc/GDD/Game_Design.md'),
    ('doc-rules', '道具与交互规则', 'Docs/02_Design_Doc/GDD/Device_Interaction_Rules.md'),
    ('doc-candidates', '候选机关道具（待选）', 'Docs/02_Design_Doc/GDD/Device_Candidates.md'),
    ('doc-level', '关卡、界面与资源规格', 'Docs/02_Design_Doc/GDD/Level_UI_Asset_Specification.md'),
    ('doc-parameters', '玩法参数基线', 'Docs/02_Design_Doc/GDD/Gameplay_Parameters.json'),
    ('doc-technical', '技术设计', 'Docs/02_Design_Doc/TDD/Technical_Design.md'),
    ('doc-decisions', '设计决策', 'Docs/02_Design_Doc/TDD/Decisions/Design_Decisions.md'),
    ('doc-coding', '代码与资产命名规范', 'Docs/03_Code_Standard/Coding_Conventions.md'),
    ('doc-version', '版本管理规范', 'Docs/03_Code_Standard/Version_Control.md'),
    ('doc-engine', 'UE 工程配置', 'Docs/04_Engine_Config/UE_Project_Configuration.md'),
    ('doc-plan', '开发任务与排期', 'Docs/05_Development_Guide/Development_Plan.md'),
    ('doc-devlog-20261016', '10/16 开发者日志', 'Docs/05_Development_Guide/Developer_Log_2026-10-16.md'),
    ('doc-audit-20261007', '10/7 进度核查', 'Docs/05_Development_Guide/Progress_Audit_2026-10-07.md'),
    ('doc-player-setup', '玩家输入与动画接入', 'Docs/05_Development_Guide/Player_Setup.md'),
    ('doc-murdock-setup', 'Murdock 与一体枪械接入', 'Docs/05_Development_Guide/Murdock_Setup.md'),
    ('doc-weapon-setup', '步枪、换弹与生命接入', 'Docs/05_Development_Guide/Weapon_Setup.md'),
    ('doc-device-setup', '机关状态与命中接入', 'Docs/05_Development_Guide/Device_Setup.md'),
    ('doc-device-refactor', '道具基础重构与调参入口', 'Docs/05_Development_Guide/Device_Refactor_Setup.md'),
    ('doc-device-interaction', '拾取、放下与安全投掷接入', 'Docs/05_Development_Guide/Device_Interaction_Setup.md'),
    ('doc-emitter-setup', 'D03 发射器与 P01 接入', 'Docs/05_Development_Guide/Emitter_Setup.md'),
    ('doc-bomb-setup', 'D02 爆炸与推力接入', 'Docs/05_Development_Guide/Bomb_Setup.md'),
    ('doc-launcher-setup', 'D01 探地与升柱接入', 'Docs/05_Development_Guide/Launcher_Setup.md'),
    ('doc-natural-chain-setup', '统一弹丸与自然连锁接入', 'Docs/05_Development_Guide/Natural_Chain_Setup.md'),
    ('doc-lifecycle-setup', '容量与生命周期接入', 'Docs/05_Development_Guide/Lifecycle_Setup.md'),
    ('doc-enemy-drops', 'T15 三外观与固定掉落接入', 'Docs/05_Development_Guide/Enemy_Drops_Setup.md'),
    ('doc-enemy-setup', 'T14 射击 AI 与导航接入', 'Docs/05_Development_Guide/Enemy_Setup.md'),
    ('doc-drongo-enemy', 'Drongo 敌人模型与动画接入', 'Docs/05_Development_Guide/Drongo_Enemy_Setup.md'),
    ('doc-wave-flow', 'T16 三波、胜负与重开接入', 'Docs/05_Development_Guide/Wave_Flow_Setup.md'),
    ('doc-ui', 'T18 HUD、教学与共用菜单接入', 'Docs/05_Development_Guide/UI_Setup.md'),
    ('doc-slate-ui', 'Slate 界面与玩家/敌人生命条', 'Docs/05_Development_Guide/Slate_UI_Setup.md'),
    ('doc-asset-import', '选定第三方资源导入', 'Docs/05_Development_Guide/Asset_Import_Setup.md'),
    ('doc-asset-shortlist', '敌人、UI 与特效资源选型', 'Docs/05_Development_Guide/Asset_Shortlist_2026-10-16.md'),
    ('doc-crosshair', '自定义准星、敌人命中与击杀反馈接入', 'Docs/05_Development_Guide/Crosshair_Setup.md'),
    ('doc-feedback', 'T19 默认反馈与替换入口', 'Docs/05_Development_Guide/Feedback_Setup.md'),
    ('doc-niagara', 'Niagara 战斗特效接入与调节', 'Docs/05_Development_Guide/Niagara_Setup.md'),
    ('doc-prototype-art', 'T20 原型外观与替换入口', 'Docs/05_Development_Guide/Prototype_Art_Setup.md'),
    ('doc-arena', 'T17 主竞技场灰盒接入', 'Docs/05_Development_Guide/Arena_Setup.md'),
    ('doc-soulcity-arena', 'Soul City 独立工业竞技场', 'Docs/05_Development_Guide/SoulCity_Arena_Setup.md'),
    ('doc-soulcity-report', 'Soul City 竞技场搭建与验证', 'Docs/06_Test_Doc/SoulCity_Arena_Report_2026-10-15.md'),
    ('doc-github', 'GitHub 开发流程', 'Docs/05_Development_Guide/GitHub_Workflow.md'),
    ('doc-workflow', '文档维护与变更流程', 'Docs/05_Development_Guide/Documentation_Workflow.md'),
    ('doc-changelog', '变更日志', 'Docs/05_Development_Guide/Changelog.md'),
    ('doc-test', '测试计划与验收', 'Docs/06_Test_Doc/Test_Plan.md'),
    ('doc-device-refactor-report', '道具基础重构验证报告', 'Docs/06_Test_Doc/Device_Refactor_Report_2026-10-14.md'),
    ('doc-t09-report', 'T09 收尾与验证报告', 'Docs/06_Test_Doc/T09_Report_2026-10-11.md'),
    ('doc-t10-report', 'T10 交付与验证报告', 'Docs/06_Test_Doc/T10_Report_2026-10-11.md'),
    ('doc-t11-report', 'T11 交付与验证报告', 'Docs/06_Test_Doc/T11_Report_2026-10-11.md'),
    ('doc-t12-report', 'T12 交付与验证报告', 'Docs/06_Test_Doc/T12_Report_2026-10-11.md'),
    ('doc-t13-report', 'T13 交付与验证报告', 'Docs/06_Test_Doc/T13_Report_2026-10-11.md'),
    ('doc-t15-report', 'T15 固定掉落交付报告', 'Docs/06_Test_Doc/T15_Report_2026-10-12.md'),
    ('doc-t14-report', 'T14 交付与验证报告', 'Docs/06_Test_Doc/T14_Report_2026-10-11.md'),
    ('doc-t16-report', 'T16 三波导演交付报告', 'Docs/06_Test_Doc/T16_Report_2026-10-12.md'),
    ('doc-t18-report', 'T18 HUD、教学与菜单交付报告', 'Docs/06_Test_Doc/T18_Report_2026-10-12.md'),
    ('doc-t19-report', 'T19 默认反馈交付报告', 'Docs/06_Test_Doc/T19_Report_2026-10-13.md'),
    ('doc-t20-report', 'T20 原型外观交付报告', 'Docs/06_Test_Doc/T20_Report_2026-10-13.md'),
    ('doc-t17-report', 'T17 主竞技场交付报告', 'Docs/06_Test_Doc/T17_Report_2026-10-12.md'),
    ('doc-release', '发布检查清单', 'Docs/07_Release/Release_Checklist.md'),
]


def source_hash(path):
    return hashlib.sha256(path.read_text(encoding='utf-8').encode('utf-8')).hexdigest()


def plain(value):
    return html.unescape(re.sub(r'<[^>]*>', '', value))


def slug(value):
    return re.sub(r'\s', '-', re.sub(r'[^\w\s-]', '', plain(value).lower()))


def render(text):
    request = {'text': text, 'mode': 'gfm', 'context': 'YYchainsAw/MothEffect'}
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', suffix='.json', delete=False) as temp:
        json.dump(request, temp, ensure_ascii=False)
        filename = temp.name
    try:
        for attempt in range(3):
            result = subprocess.run(['gh', 'api', 'markdown', '--input', filename],
                                    capture_output=True, encoding='utf-8')
            if result.returncode == 0:
                return result.stdout.replace('\r\n', '\n')
            if attempt < 2:
                time.sleep(attempt + 1)
        raise RuntimeError(result.stderr.strip())
    finally:
        Path(filename).unlink()


def fix_body(body, key, source, previous):
    # GitHub task-list checkboxes have empty IDs, which are not usable anchors.
    body = re.sub(r'\s+id=""', '', body)
    # Retain the previous snapshot's heading aliases so its existing links survive.
    legacy = {}
    for match in re.finditer(r'((?:<span id="[^"]+"></span>)*)<h([1-6])\b[^>]*>(.*?)</h\2>', previous, re.S):
        legacy.setdefault(plain(match[3]), []).extend(re.findall(r'\bid="([^"]+)"', match[0]))
    used = set()

    def heading(match):
        base = key + '-' + slug(match[2])
        anchor, count = base, 0
        while anchor in used:
            count += 1
            anchor = base + '-' + str(count)
        aliases = [value for value in legacy.get(plain(match[2]), []) if value != anchor and value not in used]
        used.update([anchor, *aliases])
        spans = ''.join(f'<span id="{html.escape(value, quote=True)}"></span>' for value in aliases)
        return f'{spans}<h{match[1]} id="{anchor}">{match[2]}</h{match[1]}>'

    body = re.sub(r'<h([1-6])\b[^>]*>(.*?)</h\1>', heading, body, flags=re.S)
    destinations = {(ROOT / name).resolve(): doc_key for doc_key, _, name in DOCS}
    destinations[OUTPUT.resolve()] = 'doc-readme'

    def link(match):
        target = html.unescape(match[2])
        parts = urlsplit(target)
        if parts.scheme or parts.netloc or not target:
            return match[0]
        if parts.path:
            base = ROOT if parts.path.startswith('/') else source.parent
            destination = (base / unquote(parts.path).lstrip('/')).resolve()
        else:
            destination = source.resolve()
        if destination in destinations:
            new = '#' + destinations[destination]
            if parts.fragment and destination != OUTPUT.resolve():
                new += '-' + unquote(parts.fragment)
        else:
            new = os.path.relpath(destination, OUTPUT.parent).replace('\\', '/')
            if parts.query:
                new += '?' + parts.query
            if parts.fragment:
                new += '#' + parts.fragment
        return match[1] + html.escape(new, quote=True) + match[3]

    return re.sub(r'(\b(?:href|src)=")([^"]*)(")', link, body)


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids, self.links, self.sections = [], [], {}

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if 'id' in attrs:
            self.ids.append(attrs['id'])
        for attribute in ['href', 'src']:
            if attribute in attrs:
                self.links.append(attrs[attribute])
        if tag == 'section' and attrs.get('class') == 'document':
            self.sections[attrs['id']] = attrs.get('data-source-sha256')


def validate(text):
    parsed = Links()
    parsed.feed(text)
    errors = []
    if len(parsed.ids) != len(set(parsed.ids)):
        errors.append('duplicate HTML anchor IDs')
    expected = {key: source_hash(ROOT / name) for key, _, name in DOCS}
    if parsed.sections != expected:
        errors.append('snapshot sections or source hashes are outdated')
    for target in parsed.links:
        parts = urlsplit(target)
        if parts.scheme or parts.netloc:
            continue
        if not parts.path:
            if parts.fragment and unquote(parts.fragment) not in parsed.ids:
                errors.append('missing anchor: ' + target)
        elif not (OUTPUT.parent / unquote(parts.path)).exists():
            errors.append('missing relative file: ' + target)
    if errors:
        raise RuntimeError('\n'.join(sorted(set(errors))))
    print(f'Reading view verified: {len(expected)} source sections; anchors and local links valid.', flush=True)


def build():
    previous = OUTPUT.read_text(encoding='utf-8')
    sections = {m[1]: m for m in re.finditer(
        r'<section class="document" id="([^"]+)"[^>]*>(.*?)</section>', previous, re.S)}
    style = re.search(r'<style>(.*?)</style>', previous, re.S)[1]
    entry = (ROOT / 'Docs/README.md').read_text(encoding='utf-8')
    version, date = re.search(r'版本 (v[\d.]+) · (\d{4}-\d{2}-\d{2})', entry).groups()
    nav = ''.join(f'<a href="#{key}">{label}</a>' for key, label, _ in DOCS)
    output = [f'''<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Moth Effect（飞蛾效应） 开发文档 {version}</title><style>{style}</style></head><body><aside><strong>Moth Effect（飞蛾效应）</strong><div class="meta">开发文档 {version}<br>{date} · 香港时间</div><nav>{nav}</nav><p class="note">魔法朋克 · UE 5.8<br>按开发阶段归档<br>Markdown/JSON 为维护源文件</p></aside><main><header><div class="meta">2026 TapTap GameJam · 单人开发</div><h1>Moth Effect（飞蛾效应） 开发文档</h1><p>第三人称越肩射击，投掷并启动机关，用连锁改变战场。</p><p class="note">此页汇总 {date} 的文档源文件；任务状态和验收证据见开发计划与测试计划。</p></header>''']
    for key, label, name in DOCS:
        source = ROOT / name
        digest = source_hash(source)
        old = sections.get(key)
        if old and f'data-source-sha256="{digest}"' in old[0]:
            output.append(old[0])
            continue
        text = source.read_text(encoding='utf-8')
        if source.suffix == '.json':
            body = f'<h1>{label}</h1>\n<pre><code>{html.escape(text)}</code></pre>\n'
        else:
            body = fix_body(render(text), key, source, old[2] if old else '')
        relative = os.path.relpath(source, OUTPUT.parent).replace('\\', '/')
        output.append(f'<section class="document" id="{key}" data-source-sha256="{digest}"><div class="source">可编辑源文件：<a href="{relative}">{name.removeprefix("Docs/")}</a></div>{body}</section>')
        print('Rendered ' + name, flush=True)
    output.append('</main></body></html>\n')
    content = ''.join(output)
    validate(content)
    OUTPUT.write_text(content, encoding='utf-8')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='check the snapshot offline without writing it')
    options = parser.parse_args()
    if options.check:
        validate(OUTPUT.read_text(encoding='utf-8'))
    else:
        build()
