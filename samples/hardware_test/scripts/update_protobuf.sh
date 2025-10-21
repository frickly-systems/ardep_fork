#!/bin/bash

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

OUTPUT_DIR="$(realpath "$SCRIPT_DIR/generated")"
PROTO_DIR="$(realpath "$SCRIPT_DIR/../src")"

echo "Generating Python gRPC code in $OUTPUT_DIR"

if [ -d "$OUTPUT_DIR" ]; then
    rm -r "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"
python3 -m grpc_tools.protoc -I "$PROTO_DIR" --python_out="$OUTPUT_DIR" --pyi_out="$OUTPUT_DIR" --grpc_python_out="$OUTPUT_DIR" "$PROTO_DIR/data.proto"



# Fix relative imports in generated Python files
# sed -i 's/import led_pb2/from . import led_pb2/g' "$OUTPUT_DIR/data_pb2.py"