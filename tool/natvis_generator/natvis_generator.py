#!/usr/bin/env python3

import jinja2
import sys

if __name__ == '__main__':

    version = 'v' + sys.argv[1].replace('.', '_')
    namespace = 'fkyaml::' + version

    print(namespace)

    environment = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath=sys.path[0]),
                                     autoescape=True,
                                     trim_blocks=True,
                                     lstrip_blocks=True,
                                     keep_trailing_newline=True)
    template = environment.get_template('fkYAML.natvis.j2')
    natvis = template.render(namespace=namespace)

    with open('fkYAML.natvis', 'w') as f:
        f.write(natvis)