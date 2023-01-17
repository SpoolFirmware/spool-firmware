#PERIPH_REGISTER_FIELD
import argparse
from typing import IO


class _HeaderWriter:
    def __init__(self, out: IO, column: int):
        self.out = out
        self.column = column

    def write_line(self, string: str):
        self.out.write(string+'\n')

    def write_right_align(self, left: str, right: str, extra: str = ''):
        extra = ' ' + " ".join(extra.split())
        spaces = ' ' * max(1, (self.column - len(left) - len(right)))
        self.write_line(f'{left}{spaces}{right}{extra}')

def _generate(input_file: str, output_file: str):
    from cmsis_svd.parser import SVDParser, SVDPeripheral, SVDRegister, SVDField
    import os.path

    parser = SVDParser.for_xml_file(input_file)

    prefix = 'DRF_'

    with open(output_file, 'w') as out:
        writer = _HeaderWriter(out, 80)
        writer.write_line(f"// Generated from {os.path.basename(input_file)}. DO NOT EDIT.\n")
        writer.write_line("#pragma once\n")
        peripheral: SVDPeripheral
        for peripheral in parser.get_device().peripherals:
            peripheral_name = peripheral.name.upper()
            writer.write_line(f'/* {peripheral_name} */')
            if peripheral._address_blocks is not None:
                block_size = peripheral._address_blocks[0].size
                writer.write_right_align(
                    f'#define {prefix}{peripheral_name}', 
                    f'0x{peripheral._base_address+block_size-1:08X}:0x{peripheral._base_address:08X}')
            reg: SVDRegister
            for reg in peripheral.registers:
                reg_description = ' '.join(reg.description.split())
                writer.write_line(f'/* {reg.name} {reg_description} */')
                writer.write_right_align(
                    f'#define {prefix}{peripheral_name}_{reg.name}',
                    f'0x{reg.address_offset+peripheral._base_address:08X}')
                writer.write_right_align(
                    f'#define {prefix}{peripheral_name}_{reg.name}_OFFSET',
                    f'0x{reg.address_offset:08X}')
                field: SVDField
                for field in reg.fields:
                    field_description = ' '.join(field.description.split())
                    writer.write_right_align(
                        f'#define {prefix}{peripheral_name}_{reg.name}_{field.name}',
                        f'{field.bit_offset+field.bit_width-1}:{field.bit_offset}',
                        f'/* {field_description} */')
                    if field.bit_width == 1:
                        writer.write_right_align(
                            f'#define {prefix}{peripheral_name}_{reg.name}_{field.name}_SET',
                            f'1U')
                        writer.write_right_align(
                            f'#define {prefix}{peripheral_name}_{reg.name}_{field.name}_CLR',
                            f'0U')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', required=True, help='Path to output .h header')
    parser.add_argument('file', help='Path to input svd file')
    args = parser.parse_args()

    _generate(args.file, args.output)

    pass

if __name__ == '__main__':
    main()