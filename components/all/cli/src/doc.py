#
# Copyright (c) 2012 Big Switch Networks, Inc.
# All rights reserved.
#

import os

#
# doc, interface to manage 'markdown' text segments based on 
# documentation tag's.
#

bigdoc = {}

def register_doc(tag, path):
    """
    Its not clear whether the registratio api ought to only name
    the available tags, or whether the api ought to also associate
    text, but it seems that it would be wiser to not keep all the
    documentation in memory
    """

    bigdoc[tag] = path

def add_doc_tags(base, tag_prefix = None):
    path = base
    if tag_prefix == None:
        tag_prefix = []
    elif type(tag_prefix) == str:
        tag_prefix = [tag_prefix]

    if os.path.exists(path):
        for elem in os.listdir(path):
            this_tag_prefix = list(tag_prefix)
            this_tag_prefix.append(elem)
            full_path = os.path.join(path, elem)
            if os.path.isdir(full_path):
                add_doc_tags(full_path, this_tag_prefix)
            elif os.path.isfile(full_path):
                register_doc('|'.join(this_tag_prefix), full_path)
            else:
                print 'add_doc_tags: unknown file type for ', full_path


def get_text(context, tag):
    path = bigdoc.get(tag)
    if path:
        if os.path.exists(path):
            with open(path, 'r') as f:
                return f.read()
        else:
            if context.bigsh.description:
                print 'doc: tag %s: missing ' % tag
            
    return ''
