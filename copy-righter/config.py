from argparse import Namespace
from dataclasses import dataclass


@dataclass
class Config:
    dry_run: bool
    verbose: bool

    @classmethod
    def from_args(cls, args: Namespace) -> "Config":
        return cls(dry_run=args.dry_run, verbose=args.verbose)
