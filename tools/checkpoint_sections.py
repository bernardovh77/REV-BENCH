"""Declared, hashed sections and diagnostic assets for the single offline result.

Sections contain already prepared listening previews. Republishing requires no
analysis packages or rerunning simulations, and never edits measurement audio.
"""
import hashlib
import json
from pathlib import Path
import shutil

SECTIONS = {
    "audio-experiment": ("AUDIO EXPERIMENT", "Research results"),
    "transport-candidates": ("ACOUSTIC CANDIDATES", "Renderer prototypes"),
    "user-references": ("USER REFERENCES", "New references"),
    "coupling-proof": ("COUPLING PROOF", "Coupling proof"),
}


def fingerprint(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def relative_file(root, name):
    path = root / name
    if Path(name).is_absolute() or '..' in Path(name).parts or path.is_symlink():
        raise ValueError("Checkpoint asset must be a local regular file")
    if not path.resolve().is_relative_to(root.resolve()) or not path.is_file():
        raise ValueError(f"Missing checkpoint asset: {name}")
    return path


def load(renders):
    path = renders / "checkpoint/manifest.json"
    if not path.exists():
        return {"schema": 1, "sections": [], "diagnostics": []}
    result = json.loads(path.read_text())
    if result.get("schema") != 1:
        raise ValueError("Unsupported checkpoint schema")
    ids = [entry["id"] for entry in result["sections"]]
    names = [entry["path"] for entry in result["diagnostics"]]
    if len(ids) != len(set(ids)) or len(names) != len(set(names)):
        raise ValueError("Duplicate checkpoint section or diagnostic")
    return result


def checked(root, entry):
    path = relative_file(root, entry["path"])
    if fingerprint(path) != entry["sha256"]:
        raise ValueError(f"Checkpoint fingerprint differs: {entry['path']}")
    return path


def save(renders, manifest):
    path = renders / "checkpoint/manifest.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.pending.json')
    temporary.write_text(json.dumps(manifest, indent=2) + '\n')
    temporary.replace(path)


def retained_diagnostics(renders):
    result=[]
    for path in sorted((renders / 'current/experiment').rglob('*')):
        if path.is_file():
            name=str(path.relative_to(renders/'current'))
            relative_file(renders/'current',name)
            result.append(dict(path=name,sha256=fingerprint(path)))
    return result


def adopt_current(renders):
    """One-time migration of known sections and retained diagnostic inventory."""
    if (renders / "checkpoint/manifest.json").exists():
        raise ValueError("Checkpoint already registered; use normal republication")
    page = (renders / "RESULTS.html").read_text()
    manifest = load(renders)
    for key, (marker, label) in SECTIONS.items():
        begin, end = f'<!-- {marker} BEGIN -->', f'<!-- {marker} END -->'
        if begin not in page and end not in page:
            continue
        if page.count(begin) != 1 or page.count(end) != 1:
            raise ValueError(f"Malformed checkpoint section: {key}")
        fragment = begin + page.split(begin, 1)[1].split(end, 1)[0] + end
        destination = renders / f"checkpoint/sections/{key}.html"
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(fragment)
        manifest['sections'].append(dict(id=key, marker=marker, label=label,
            path=str(destination.relative_to(renders)), sha256=fingerprint(destination)))
    manifest['diagnostics']=retained_diagnostics(renders)
    save(renders, manifest)
    return manifest


def compose(page, renders, manifest=None):
    manifest = load(renders) if manifest is None else manifest
    if page.count('</main>') != 1 or page.count('</nav>') != 1:
        raise ValueError("Expected one results main and navigation element")
    for entry in manifest['sections']:
        fragment = checked(renders, entry).read_text()
        begin, end = f"<!-- {entry['marker']} BEGIN -->", f"<!-- {entry['marker']} END -->"
        if fragment.count(begin) != 1 or fragment.count(end) != 1:
            raise ValueError("Malformed registered section")
        if begin in page or end in page:
            if page.count(begin) != 1 or page.count(end) != 1:
                raise ValueError("Malformed existing section")
            before, rest = page.split(begin, 1)
            page = before + rest.split(end, 1)[1].removeprefix('\n')
        link = f'<a href="#{entry["id"]}">{entry["label"]}</a>'
        page = page.replace(link, '')
        page = page.replace('</main>', fragment + '\n</main>', 1)
        page = page.replace('</nav>', link + '</nav>', 1)
    return page


def diagnostic_names(current, renders):
    manifest = load(renders)
    for entry in manifest['diagnostics']:
        checked(current, entry)
    return {entry['path'] for entry in manifest['diagnostics'] if entry['path'].endswith('.wav')}


def carry_diagnostics(current, staged, renders):
    for entry in load(renders)['diagnostics']:
        source = checked(current, entry)
        target = staged / entry['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            if fingerprint(target) != entry['sha256']:
                raise ValueError("Conflicting staged diagnostic fingerprint")
        else:
            shutil.copyfile(source, target)


def publish_section(target, key, fragment):
    """Register one producer's result and compose all declared sections."""
    renders = target.parent
    if not (renders / 'checkpoint/manifest.json').exists():
        adopt_current(renders)
    manifest = load(renders)
    previous_manifest = (renders / 'checkpoint/manifest.json').read_bytes()
    marker, label = SECTIONS[key]
    destination = renders / f'checkpoint/sections/{key}.html'
    destination.parent.mkdir(parents=True, exist_ok=True)
    previous = destination.read_bytes() if destination.exists() else None
    destination.write_text(fragment)
    entry = dict(id=key, marker=marker, label=label,
                 path=str(destination.relative_to(renders)), sha256=fingerprint(destination))
    manifest['sections'] = [row for row in manifest['sections'] if row['id'] != key] + [entry]
    if key=='audio-experiment':
        manifest['diagnostics']=retained_diagnostics(renders)
    temporary = target.with_suffix('.checkpoint.pending.html')
    try:
        temporary.write_text(compose(target.read_text(), renders, manifest))
        save(renders, manifest)
        try:
            temporary.replace(target)
        except BaseException:
            (renders / 'checkpoint/manifest.json').write_bytes(previous_manifest)
            raise
    except BaseException:
        if previous is not None:
            destination.write_bytes(previous)
        raise
    finally:
        temporary.unlink(missing_ok=True)
