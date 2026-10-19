#!/usr/bin/env python3
"""Valide les noms utilisés avec la bibliothèque CeXii MQTT avant d'écrire le code.

Exemples :
  python3 check_names.py device controlbouton
  python3 check_names.py username esp32poeiso controlbouton
  python3 check_names.py segment lumiere bande1 couleur
  python3 check_names.py key r g b
  python3 check_names.py topic globale lumiere bande1 couleur
  python3 check_names.py topic globale lumiere bande1 couleur --config ../../../src/CexiiMqttConfig.h

Code de sortie : 0 si tout est valide, 1 sinon. Les règles reprennent src/CexiiMqttConfig.h.
"""
import argparse
import re
import sys

NAME_MAX, SEGMENT_MAX, KEY_MAX = 24, 12, 6
DEFAULT_ZONES = ["jam", "ecrans", "globale", "regie"]
BOARDS = {"esp32": "esp32", "esp32poe": "esp32poe", "esp32poeiso": "esp32poeiso"}


def check(value, kind):
    """Retourne un message d'erreur, ou None si la valeur est valide."""
    rules = {
        "device": (NAME_MAX, r"[A-Za-z0-9]+", "lettres et chiffres seulement"),
        "segment": (SEGMENT_MAX, r"[A-Za-z0-9_-]+", "lettres, chiffres, _ ou -"),
        "key": (KEY_MAX, r"[A-Za-z0-9_]+", "lettres, chiffres ou _"),
    }
    maxlen, pattern, allowed = rules[kind]
    if not value:
        return "vide"
    if len(value) > maxlen:
        return f"{len(value)} caractères (max {maxlen})"
    if not re.fullmatch(pattern, value):
        return f"caractère interdit ({allowed})"
    return None


def read_zones(config_path):
    text = open(config_path, encoding="utf-8").read()
    block = re.search(r"#define\s+\w*ZONES\(X\)(.*?)\n\s*\n", text, re.S)
    zones = re.findall(r'X\(\s*\w+\s*,\s*"([^"]+)"\s*\)', block.group(1) if block else text)
    return zones or DEFAULT_ZONES


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("kind", choices=["device", "username", "segment", "key", "topic"])
    p.add_argument("values", nargs="+")
    p.add_argument("--config", help="chemin de CexiiMqttConfig.h pour lire la liste des zones")
    args = p.parse_args()

    problems = []

    def verify(value, kind, label):
        err = check(value, kind)
        print(f"{'OK  ' if err is None else 'ERREUR'} {label} \"{value}\"" + ("" if err is None else f" : {err}"))
        if err:
            problems.append(label)

    if args.kind in ("device", "segment", "key"):
        for v in args.values:
            verify(v, args.kind, {"device": "nom d'appareil", "segment": "segment", "key": "clé"}[args.kind])
    elif args.kind == "username":
        if len(args.values) != 2 or args.values[0].lower() not in BOARDS:
            sys.exit("usage : username <esp32|esp32poe|esp32poeiso> <nom>")
        verify(args.values[1], "device", "nom d'appareil")
        if not problems:
            print(f"username : {BOARDS[args.values[0].lower()]}_{args.values[1].lower()}")
    else:  # topic
        if len(args.values) != 4:
            sys.exit("usage : topic <zone> <module> <composante> <propriété>")
        zone, *segments = args.values
        zones = read_zones(args.config) if args.config else DEFAULT_ZONES
        if zone.lower() not in zones:
            print(f"ERREUR zone \"{zone}\" : pas dans la liste ({', '.join(zones)})")
            problems.append("zone")
        for label, v in zip(["module", "composante", "propriété"], segments):
            verify(v, "segment", label)
        if not problems:
            print("topic : saillance/" + "/".join(v.lower() for v in args.values))
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
