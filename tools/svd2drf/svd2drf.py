#PERIPH_REGISTER_FIELD
import argparse
from typing import IO
from cmsis_svd.parser import SVDParser
from cmsis_svd.model import *
import re


class CHeaderWriter:
    def __init__(self, out: IO, column: int = 80, define_prefix="DRF_"):
        self.prefix = define_prefix
        self.out = out
        self.column = column

    def write_line(self, string: str):
        self.out.write(string+'\n')

    def write_right_align(self, left: str, right: str, extra: str = ''):
        extra = ' ' + " ".join(extra.split())
        spaces = ' ' * max(1, (self.column - len(left) - len(right)))
        self.write_line(f'{left}{spaces}{right}{extra}')

    def write_define(self, name, content, comment=None):
        self.write_right_align(
            f'#define {self.prefix}{name}',
            content,
            '' if comment is None else f'/* {comment} */'
        )

def camel_to_snake(name):
    name = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', name).upper()


class DRFHeaderGenerator:
    def __init__(self, input_file: str, output_file: str) -> None:
        self.output_file = open(output_file, 'w')
        self.out = CHeaderWriter(self.output_file)
        self.device = SVDParser.for_xml_file(input_file).get_device()
        pass

    def processField(self, p: SVDPeripheral, r: SVDRegister, f: SVDField):
        pr_name = f'{p.name}_{r.name}'
        fld_name: str
        fld_name = f.name
        if f.dim is not None:
            dim_idx_cnt = len(f.dim_index_text)
            fld_name = f.name.replace('%s', '')
            assert f.dim_increment is not None
            idx_name = f.name.replace('%s', '(n)')
            offset_component = '' if f.bit_offset == 0 else f'{f.bit_offset} + '
            self.out.write_define(
                f'{pr_name}_{idx_name}', 
                f'({offset_component}{f.bit_width-1} + ((n) * {f.dim_increment})):({offset_component}((n) * {f.dim_increment}))', 
                f'{f.dim_index_text}')
            self.out.write_define(
                f'{pr_name}_{fld_name}__COUNT',
                f'{dim_idx_cnt}U'
            )
        else:
            self.out.write_define(
                f'{pr_name}_{fld_name}',
                f'{f.bit_offset+f.bit_width-1}:{f.bit_offset}'
            )
        
        # Enumerate
        if f.enumerated_values is not None:
            e: SVDEnumeratedValue
            for e in f.enumerated_values:
                self.out.write_define(
                    f'{pr_name}_{fld_name}_{camel_to_snake(e.name)}',
                    f'{e.value}U',
                    f'{e.description}'
                )
        elif f.bit_width == 1:
            values = []
            if fld_name.endswith('EN'):
                values.append((0, 'DISABLED'))
                values.append((1, 'ENABLED'))
            elif fld_name.endswith('RST'):
                values.append((0, 'CLR'))
                values.append((1, 'RESET'))
            else:
                values.append((0, 'CLR'))
                values.append((0, 'SET'))
            for (v, name) in values:
                self.out.write_define(
                    f'{pr_name}_{fld_name}_{name}',
                    f'{v}U')

        pass

    def processRegister(self, p: SVDPeripheral, r: SVDRegister):
        self.out.write_define(f'{p.name}_{r.name}', f'0x{r.address_offset:08X}', r.description)
        if r.fields is not None:
            for f in r.fields:
                self.processField(p,r,f)
        pass

    def processPeripheral(self, p: SVDPeripheral):
        self.out.write_line(
            f'/**\n'+
            f' * @defgroup {p.name} {p.description}\n'
            f' * @{{\n'+
            f' */')
        if p.base_address is not None:
            block_size = 0;
            if p.address_blocks is not None:
                block_size = p.address_blocks[0].size
            else:
                print(f'WARN: {p.name} has base_address defined but no block')
            self.out.write_define(f'{p.name}', f'0x{p.base_address+block_size:08X}:0x{p.base_address:08X}')
        
        for r in p.registers:
            self.processRegister(p, r)
        
        self.out.write_line(f'/*! @}}*/\n')
        pass

    def generate(self):
        if self.output_file is None:
            raise Exception('Invalid State for Generate')

        # Write headers
        self.out.write_line(f"// Generated from (patched) SVD files. DO NOT EDIT.")
        self.out.write_line("#pragma once\n")

        for peripheral in self.device.peripherals:
            self.processPeripheral(peripheral)
        
        self.output_file.close()
        self.output_file = None

def generate_header(input_file: str, output_file: str):
    gen = DRFHeaderGenerator(input_file, output_file)
    gen.generate()
    

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', required=True, help='Path to output .h header')
    parser.add_argument('file', help='Path to input svd file')
    args = parser.parse_args()

    generate_header(args.file, args.output)

    pass

if __name__ == '__main__':
    main()