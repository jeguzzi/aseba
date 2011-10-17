#   Aseba - an event-based framework for distributed robot control
#   Copyright (C) 2007--2011:
#           Stephane Magnenat <stephane at magnenat dot net>
#           (http://stephane.magnenat.net)
#           and other contributors, see authors.txt for details
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, version 3 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with this program. If not, see <http://www.gnu.org/licenses/>.

# Python lib
import sys
from myparser import MyParser
from string import Template

# Local module
import wikidot.debug

header = \
"""
<!DOCTYPE html
     PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
     "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
<link rel='stylesheet' type='text/css' href='aseba.css' />
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
<title>${title}</title>
</head >

<body>
"""

footer = \
"""
</body>
</html>
"""

class WikidotParser(MyParser):
    """WikidotParser is used to clean a page from www.wikidot.com,
    keeping only the interesting content."""
    def __init__(self):
        """Intialize internal variables"""
        MyParser.__init__(self)
        self.div_level = 0
        self.div_bookmark = [-1]    # List managed as a stack
        self.state = ["none"]       # List managed as a stack
        # map for div tag attribute -> state
        # (attribute name, attribute property, state)
        self.div_state_map = \
            [
            ('id', 'page-title', 'title'),
            ('id', 'breadcrumbs', 'breadcrumbs'),
            ('id', 'page-content', 'body'),
            ('id', 'toc', 'toc'),
            ('style','position:absolute', 'useless')]
        self.page_title = ""
        self.links = set()
        self.breadcrumbs = list()

    # Public interface
    def get_doc(self):
        """Retrieve the parsed and cleaned document"""
        # Add header
        header_template = Template(header)
        self.out_doc = header_template.substitute(title=self.page_title) + self.out_doc
        # Add footer
        self.out_doc += footer
        return self.out_doc

    def get_links(self):
        """Retrieve the links embedded in the page (including images)"""
        return self.links

    def get_title(self):
        return self.page_title

    def get_breadcrumbs(self):
        return self.breadcrumbs

    # Inherited functions
    def handle_starttag(self, tag, attrs):
        """Overridden - Called when a start tag is parsed

        The heart of this function is the state machine.
        When a <div> tag is detected, the attributes are compared with
        a map of the form (name,value) -> state. If a match occurs,
        the state is pushed on top of the stack.

        Depending on the current state, the start tag is queued for output,
        or not."""
        if wikidot.debug.ENABLE_DEBUG == True:
            print >> sys.stderr, "<{}> {}".format(tag, attrs)

        # Output here: reference tag will NOT appear in the output
        # BUG: tag still appear if part of nested tags
        if self.state[-1] == "body":
            # Special case 1: links
            if tag == 'a':
                for index, attr in enumerate(attrs):
                    if attr[0] == 'href':
                        self.links.add(attr[1])
                        break
            # Special case 2: images
            elif tag == 'img':
                for index, attr in enumerate(attrs):
                    if attr[0] == 'src':
                        self.links.add(attr[1])
                    elif attr[0] == 'width':
                        # Fix the width=xx attribute
                        # Wikidot gives width="600px", instead of width=600
                        pos = attr[1].find('px')
                        if pos >= 0:
                            attrs[index] = (attr[0], attr[1][0:pos])

            # Add the tag to output
            MyParser.handle_starttag(self, tag, attrs)
        # Handle breadcrumbs
        elif (self.state[-1] == "breadcrumbs") and (tag == 'a'):
            # Register the breadcrumbs
            for attr in attrs:
                if (attr[0] == 'href'):
                    self.breadcrumbs.append(attr[1])
                    break

        # Update the state machine
        if tag == 'div':
            if wikidot.debug.ENABLE_DEBUG == True:
                print >> sys.stderr, self.state, self.div_bookmark
            # Look for the id = xyz attribute
            for attr in attrs:
                for div_attr in self.div_state_map:
                    if (div_attr[0] == attr[0]) and (div_attr[1] in attr[1]):
                        # Match !
                        self.state.append(div_attr[2])
                        self.div_bookmark.append(self.div_level)
                        break
            # Increment div level
            self.div_level += 1

        # Output here: reference tag will appear in the output
        # None

    def handle_endtag(self, tag):
        """Overridden - Called when an end tag is parsed

        The state machine is updated when a </div> tag is encountered.
        Depending on the current state, the end tag is queued for output,
        or not."""
        # Output here: reference tag will appear in the output

        # Update the state machine
        if tag == 'div':
            if wikidot.debug.ENABLE_DEBUG == True:
                print >> sys.stderr, self.state, self.div_bookmark
            self.div_level -= 1
            if self.div_level == self.div_bookmark[-1]:
                # Matching closing </div> tag -> pop the state
                self.state.pop()
                self.div_bookmark.pop()

        # Output here: reference tag will NOT appear in the output
        if self.state[-1] == "body":
            # Add the tag to output
            MyParser.handle_endtag(self, tag)

    def handle_data(self, data):
        """Overridden - Called when some data is parsed

        Depending on the current state, the data is queued for output,
        or not."""
        if self.state[-1] == "title":
            self.page_title += data.strip()
        elif self.state[-1] == "body":
            MyParser.handle_data(self, data)

    def handle_charref(self, name):
        """Overridden - Called when a charref (&#xyz) is parsed

        Depending on the current state, the charref is queued for output,
        or not."""
        if self.state[-1] == "body":
            MyParser.handle_charref(self, name)

    def handle_entityref(self, name):
        """Overridden - Called when an entityref (&xyz) tag is parsed

        Depending on the current state, the entityref is queued for output,
        or not."""
        if self.state[-1] == "body":
            MyParser.handle_entityref(self, name)

    def handle_decl(self, decl):
        """Overridden - Called when a SGML declaration (<!) is parsed

        Depending on the current state, the declaration is queued for output,
        or not."""
        if self.state[-1] == "body":
            MyParser.handle_decl(self, decl)

