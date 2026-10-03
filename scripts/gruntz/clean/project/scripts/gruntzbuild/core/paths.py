"""Paths owned by the standalone source project."""
import os
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
INCLUDE = REPO / 'include'
VENDOR = REPO / 'vendor'


def msvc_dir():
    return Path(os.environ['MSVC_DIR'])


def dxsdk_dir():
    return Path(os.environ['DXSDK_DIR'])
