#!/usr/bin/python
# This file is part of Te Tuhi Video Game System.
#
# Copyright (C) 2008 Douglas Bagnall
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import Image

im = Image.open('/home/douglas/images/tetuhi-examples/cowboy-tanks.jpg-sprites/alpha-2.png')

w, h = im.size
print im.size

im.save('/tmp/orig.png')
im2 = im.transform(im.size, Image.EXTENT, (2,50,120,123))
im2.save('/tmp/extent.png')

im3 = im.transform(im.size, Image.AFFINE, (2,-1,0,-1,2,0))
im3.save('/tmp/affine1.png')

im3 = im.transform(im.size, Image.AFFINE, (0,1,0,1,0,0))
im3.save('/tmp/affine2.png')
im3 = im.transform(im.size, Image.AFFINE, (1.5,-0.5,0,-0,1,0))
im3.save('/tmp/affine3.png')

im4 = im.transform(im.size, Image.QUAD, (0,0, 0,h, w,h, w,0))
im4.save('/tmp/quad1.png')

im4 = im.transform(im.size, Image.QUAD, (0,0, 0,h, w,0, w,h))
im4.save('/tmp/quad2.png')

im4 = im.transform(im.size, Image.QUAD, (0,h, w,0, 0,0, w,h))
im4.save('/tmp/quad3.png')

w1 = w // 4
w3 = w * 3 // 4
h1 = h // 4
h3 = h * 3 // 4

im4 = im.transform(im.size, Image.QUAD, (w3,h3, w3,w1, w1,h3, w1,h1))
im4.save('/tmp/quad4.png')

im4 = im.transform(im.size, Image.QUAD, (w*2,-h, w*2,h*2, -w,h*2, -w,-h))
im4.save('/tmp/quad5.png')

im4 = im.transform(im.size, Image.QUAD, (w*2,-h, -w,-h, w*2,h*2, -w,h*2))
im4.save('/tmp/quad6.png')

#mesh
im5 = im.transform(im.size, Image.MESH, [((w1,h1, w3,h3), (0,0, w,0, w,h, 0,h))]
                   )
im5.save('/tmp/mesh1.png')
im5 = im.transform(im.size, Image.MESH, [((w1,h1, w3,h3), (0,0, -w,0, -w,-h, 0,-h)),
                                         ]

                                         )
im5.save('/tmp/mesh2.png')
im5 = im.transform(im.size, Image.MESH, [((0,0,w//2,h//2), (0,0,  w,h, w,0, 0,h)),
                                         ((w//2,0, w,h//2), (0,0, w,0, w,h, 0,h)),
                                         ((w//2,h//2, w,h), (0,0, w,0, w,h, 0,h)),
                                         ((0,h//2, w//2,h), (0,0, w,0, w,h, 0,h)),
                                         ]
                                         )
im5.save('/tmp/mesh3.png')
im5 = im.transform((w*2, h*2), Image.MESH, [((0,0,w,h), (0,h,  w,h, w,0, 0,0)),
                                            ((w,0, w*2,h), (w,0, w,h, 0,h, 0,0)),
                                            ((w,h, w*2,h*2), (w,h, 0,h, 0,0, w,0)),
                                            ((0,h, w,h*2), (0,0, 0,h, w,h, w,0)),
                                            ]
                                         )
im5.save('/tmp/mesh4.png')

im5 = im.transform((w*2, h*2), Image.MESH, [((0,0,w,h1), (32,0, 0,h1, w,h1, w-32,0)),
                                            ((0,h1,w,h3), (0,h1, 0,h3, w,h3, w,h1)),
                                            ((0,h3,w,h), (0,h3, 32,h, w-32,h, w,h3)),
                                            
                                            ((w,0, w*2,h), (0,0, 0,h, w,h, w,0)),
                                            ((w-16,h-16, w*2,h*2), (0,0, 0,h, w,h, w,0)),
                                            ((0,h, w,h*2), (0,0, 0,h, w,h, w,0)),
                                            ]
                                         )
im5.save('/tmp/mesh5.png')


