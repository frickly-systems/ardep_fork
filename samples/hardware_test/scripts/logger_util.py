import logging

APP_PREFIX = "CES2026"  # pick any constant


def get_logger(name: str) -> logging.Logger:
    return logging.getLogger(f"{APP_PREFIX}.{name}")
