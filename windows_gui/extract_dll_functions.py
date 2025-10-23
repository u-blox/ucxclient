"""Extract all exported function names from Windows DLL"""
import os
import sys
import struct

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def get_dll_exports_from_file(dll_path):
    """Parse PE file to extract exported function names"""
    exports = []
    
    try:
        with open(dll_path, 'rb') as f:
            # Read DOS header
            dos_header = f.read(64)
            if dos_header[:2] != b'MZ':
                print("Not a valid PE file")
                return exports
            
            # Get PE header offset
            pe_offset = struct.unpack('<I', dos_header[60:64])[0]
            
            # Read PE signature
            f.seek(pe_offset)
            pe_sig = f.read(4)
            if pe_sig != b'PE\x00\x00':
                print("Invalid PE signature")
                return exports
            
            # Read COFF header
            coff_header = f.read(20)
            machine, num_sections, _, _, _, opt_header_size, characteristics = struct.unpack('<HHIIIHH', coff_header)
            
            # Read Optional header
            opt_header_start = f.tell()
            magic = struct.unpack('<H', f.read(2))[0]
            is_64bit = (magic == 0x20b)
            
            # Skip to data directories
            if is_64bit:
                f.seek(opt_header_start + 112)
            else:
                f.seek(opt_header_start + 96)
            
            # Read export table RVA and size (first data directory)
            export_table_rva, export_table_size = struct.unpack('<II', f.read(8))
            
            if export_table_rva == 0:
                print("No export table found")
                return exports
            
            # Read section headers to find where export table is
            f.seek(opt_header_start + opt_header_size)
            sections = []
            for i in range(num_sections):
                section = f.read(40)
                name = section[:8].rstrip(b'\x00').decode('ascii', errors='ignore')
                virtual_size, virtual_address, raw_size, raw_offset = struct.unpack('<IIII', section[8:24])
                sections.append((name, virtual_address, raw_size, raw_offset))
            
            # Find section containing export table
            export_section = None
            for name, vaddr, rsize, roffset in sections:
                if vaddr <= export_table_rva < vaddr + rsize:
                    export_section = (vaddr, roffset)
                    break
            
            if not export_section:
                print("Could not find export section")
                return exports
            
            # Calculate file offset of export table
            export_table_offset = export_table_rva - export_section[0] + export_section[1]
            
            # Read export directory table
            f.seek(export_table_offset)
            export_dir = f.read(40)
            _, _, _, name_rva, base, num_functions, num_names, addr_table_rva, name_ptr_table_rva, ordinal_table_rva = struct.unpack('<IIIIIIIIII', export_dir)
            
            # Read name pointer table
            name_ptr_table_offset = name_ptr_table_rva - export_section[0] + export_section[1]
            f.seek(name_ptr_table_offset)
            name_ptrs = [struct.unpack('<I', f.read(4))[0] for _ in range(num_names)]
            
            # Read each function name
            for name_rva in name_ptrs:
                name_offset = name_rva - export_section[0] + export_section[1]
                f.seek(name_offset)
                name_bytes = b''
                while True:
                    byte = f.read(1)
                    if byte == b'\x00':
                        break
                    name_bytes += byte
                name = name_bytes.decode('ascii', errors='ignore')
                if name.startswith('uCx'):
                    exports.append(name)
            
    except Exception as e:
        print(f"Error parsing DLL: {e}")
        import traceback
        traceback.print_exc()
    
    return exports

if __name__ == "__main__":
    from ucx_wrapper import UcxClientWrapper
    
    wrapper = UcxClientWrapper()
    dll_path = wrapper._dll._name
    
    print(f"Parsing DLL: {dll_path}")
    print("="*60)
    
    exports = get_dll_exports_from_file(dll_path)
    
    print(f"\nFound {len(exports)} uCx* functions:")
    print("="*60)
    for func in sorted(exports):
        print(f"  {func}")
    
    # Save to file for mapper to use
    output_file = os.path.join(os.path.dirname(__file__), 'dll_exports.txt')
    with open(output_file, 'w') as f:
        for func in sorted(exports):
            f.write(func + '\n')
    
    print(f"\n✓ Saved to {output_file}")
