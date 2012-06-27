#
# 	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>
#
#	This file is part of touchstream.
#
#	touchstream is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	touchstream is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with touchstream.  If not, see <http://www.gnu.org/licenses/>.

# you need Graphviz, ruby and exuberant ctags here. 
# this is not generated in the normal code build

all:	callgraph.pdf

callgraph.pdf:
	ctags -f .tags ../src/*.[ch] ../src/*/*.[ch] 2>/dev/null && \
		ruby ./tags_to_dot.rb .tags \
			../src/*.c ../src/*/*.c >.tags.dot && \
				dot -Tpdf .tags.dot -o $@

clean:
	rm -f .tags*
