#!/usr/bin/env python3
"""Simple package manager for Orus scripts."""
import argparse
import json
import os
import shutil
import subprocess
import tarfile

def init_project(args):
    manifest = {
        "name": args.name,
        "version": "0.1.0",
        "entry": "src/main.orus",
        "dependencies": {}
    }
    os.makedirs("src", exist_ok=True)
    main_path = os.path.join("src", "main.orus")
    if not os.path.exists(main_path):
        with open(main_path, "w") as f:
            f.write('print("Hello from Orus!")\n')
    with open("orus.json", "w") as f:
        json.dump(manifest, f, indent=2)
    print("Initialized new Orus package")

def build(args):
    subprocess.run(["make"], check=True)

def run(args):
    build(args)
    if os.path.exists("orus.json"):
        with open("orus.json") as f:
            data = json.load(f)
            entry = data.get("entry")
    else:
        entry = None

    if entry:
        subprocess.run(["./orus", entry])
    else:
        subprocess.run(["./orus", "--project", "."])

def pack(args):
    build(args)
    if not os.path.exists("orus.json"):
        raise SystemExit("Manifest orus.json not found")
    with open("orus.json") as f:
        data = json.load(f)
    dist_dir = "dist"
    os.makedirs(dist_dir, exist_ok=True)
    pkg_name = f"{data['name']}-{data['version']}.tar.gz"
    pkg_path = os.path.join(dist_dir, pkg_name)
    with tarfile.open(pkg_path, "w:gz") as tar:
        tar.add("orus")
        tar.add("src")
        tar.add("orus.json")
    print(f"Created package {pkg_path}")

def main():
    parser = argparse.ArgumentParser(description="Orus package manager")
    sub = parser.add_subparsers(dest="command")

    init_p = sub.add_parser("init", help="Initialize a new project")
    init_p.add_argument("name")
    init_p.set_defaults(func=init_project)

    build_p = sub.add_parser("build", help="Compile the Orus interpreter")
    build_p.set_defaults(func=build)

    run_p = sub.add_parser("run", help="Build and run the package entrypoint")
    run_p.set_defaults(func=run)

    pack_p = sub.add_parser("pack", help="Create a distributable archive")
    pack_p.set_defaults(func=pack)

    args = parser.parse_args()
    if hasattr(args, "func"):
        args.func(args)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()
