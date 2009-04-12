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
#
# see the README file in the perceptron directory for information
# about possible future licensing.

import random, time, struct
from array import array
from nnpy import Network, test_buffer


def test_set(net, set, binary=1):
    """compares and prints a net's outputs against targets"""
    for i, o  in set:
        b = net.opinion(i)
        if binary:
            c = [ int(round(x)) for x in b  ]
        else:
            c = ', '.join([ "%0.2f" % x for x in b ])

        ok = ('bad!', '*')[c == o]
        print "in  %06s   out  %s  wanted   %s   %s" % (i, c, o, ok)

def compare_nets(a, b, shape, sample=6):
    for z in range(sample):
        ins = [ random.randrange(2) for x in range(shape[0]) ]
        ao = [ "%0.2f" %x for x in a.opinion(ins) ]
        bo = [ "%0.2f" %x for x in b.opinion(ins) ]
        ok = ('bad!', '*')[ao == bo]
        print "in  %06s   out  %s  wanted   %s   %s" % (ins, ao, bo, ok)




def learn_set(sizes, set, binary=1, learn_rate=0.16, momentum=0.3, bias=0,
              print_every=0, temperature=55.0, sustain=0.9992, min_temp=0.002,
              method='backprop', threshold=0.001):
    """creates a Network and tests it against a training set,
    returning the network.
    Training set should be in the form [ (inputs, targets), ...] """
    net = Network(sizes, bias=bias)
    net.randomise([-0.5, 0.5], 1)
    print "training with %s" % method
    if method == 'backprop':
        net.train_set(set, count=80000, threshold=threshold,
                      learn_rate=learn_rate, momentum=momentum, print_every=print_every)
    elif method == 'anneal':
        net.anneal_set(set, count=80000, threshold=threshold,
                       temperature=temperature, sustain=sustain, min_temp=min_temp,
                       print_every=print_every)

    test_set(net, set, binary)
    return net

def learn_xor(method='backprop'):
    set = [([0,0],[0]), ([1,0],[1]), ([0,1],[1]), ([1,1],[0]), ]
    a = learn_set([2,3,1], set, print_every=10000, method=method)
    return a

def learn_xor_neg(method='backprop'):
    set = [([0,0],[0]), ([-1,0],[1]), ([0,-1],[1]), ([-1,-1],[0]), ]
    a = learn_set([2,4,1], set, method=method)
    return a

def learn_count(method='backprop'):
    # count function ?
    set = [
        [[0,0,0], [0,0,0]],
        [[0,0,1], [1,1,0]],
        [[0,1,0], [1,1,0]],
        [[0,1,1], [0,1,1]],
        [[1,0,0], [1,1,0]],
        [[1,0,1], [0,1,1]],
        [[1,1,0], [0,1,1]],
        [[1,1,1], [1,1,1]]
    ]
    return learn_set([3,7,3], set, method=method)

def learn_symmetry(method='backprop', bias=1):
    set = []
    shape = [5, 4, 3, 1]
    for x in range(32):
        i = [int(x & 16 > 0), int(x & 8 > 0), int(x & 4 > 0), int(x & 2 > 0), int(x & 1 > 0)]
        o = [int(i[0] == i[4] and i[1] == i[3])]
        set.append((i,o))
    subset = random.sample(set, 24)
    if method in ('anneal', 'both'):
        net = learn_set(shape, subset, method='anneal',
                        temperature=25.0, sustain=0.9981, min_temp=0.002,
                        threshold=0.001)
        print "testing on untrained"
        test_set(net, [x for x in set if x not in subset])

    if method in ('backprop', 'both'):
        net = learn_set(shape, subset)
        print "testing on untrained"
        test_set(net, [x for x in set if x not in subset])

def bitwise_list(n, size=0):
    bits = []
    if not size:
        while n:
            bits.append(n & 1)
            n >>= 1
    for x in range(size):
        bits.append(n & 1)
        n >>= 1
    bits.reverse()
    return bits

def learn_best_of_set(arrays=0):
    sets = []
    SIZE = 4
    iterations = 200
    net = Network([SIZE, SIZE, 1], bias=1)
    format = '....f...d'[net.weight_size]
    noisy = 0

    for i in range(128):
        n = random.sample(range(2 ** SIZE), 4)
        n.sort()
        s = [bitwise_list(x, SIZE) for x in n ]
        if arrays:
            s = array(format, sum(s, []))
        sets.append(s)

    print sets[0]
    print len(sets[0])
    t2 = time.time()
    net.randomise([-0.5, 0.5], 1)
    for k in range(iterations):
        t = time.time()
        net.randomise([-0.5, 0.5])
        diff = net.best_of_set(sets, count=5000)
        if noisy:
            print "diff was %s; took %f seconds" % (diff, time.time() - t)
    print time.time() - t2
    t2 = time.time()
    print "doing genetics"
    net.randomise([-0.5, 0.5], 1)
    for k in range(iterations):
        t = time.time()
        net.randomise([-0.5, 0.5])
        diff = net.best_of_set(sets, count=1000, population=16)
        if noisy:
            print "diff was %s; took %f seconds" % (diff, time.time() - t)
    print time.time() - t2


    inputs = [bitwise_list(x, SIZE) for x in range(2 ** SIZE)]

    if noisy:
        y = 1e100
        for x in inputs:
            if arrays:
                o = net.opinion(array(format, x))[0]
            else:
                o = net.opinion(x)[0]

            ok = ('', '^')[o > y]
            y = o
            print "%30s %20s %s" %(x, o, ok)



def test_save(func=learn_xor):
    fn = '/tmp/nn.weights'
    a = func()
    z = a.save_weights(fn)
    print "saved weights in %s"  %fn
    print "Now trying b, loaded therefrom"
    b = Network(a.shape)
    b.load_weights(fn)
    for z in range(6):
        ins = [ random.randrange(2) for x in range(3) ]
        ao = [ "%0.2f" %x for x in a.opinion(ins) ]
        bo = [ "%0.2f" %x for x in b.opinion(ins) ]
        ok = ('bad!', '*')[ao == bo]
        print "in  %06s   out  %s  wanted   %s %s" % (ins, ao, bo, ok)

def test_weight_strings(func=learn_count):
    a = func()
    s = a.weights
    print "weights are type %s, len %s"  % (type(s), len(s))
    #print [ord(x) for x in s ]
    b = Network(a.shape)
    b.weights = a.weights
    compare_nets(a, b, a.shape)

def test_opinion_strings():
    net = learn_xor('backprop')
    set = [([0,0],[0]), ([1,0],[1]), ([0,1],[1]), ([1,1],[0]), ]
    for t in set:
        inputf = [float(x) for x in t[0]]
        s = struct.pack("2d", *inputf)
        print [ord(x) for x in s]
        print t, net.opinion(s)

def test_opinion_arrays():
    net = learn_xor('backprop')
    set = [([0,0],[0]), ([1,0],[1]), ([0,1],[1]), ([1,1],[0]), ]
    for t in set:
        inputf = [float(x) for x in t[0]]
        s = struct.pack("2d", *inputf)
        a = array('c', s)
        print a
        print t, net.opinion(a)



def test_arrays():
    s = [float(x) for x in range(5) ]
    a = array('d', s)
    b = buffer(a)
    c = a.tostring()

    for x in a, b, c:
        print dir(x)
        print len(x)
        print test_buffer(x)
        print


def test_pickle():
    import cPickle
    for shape in ([2,3,1], [4,6,7]):
        net1 = Network(shape, bias=1)
        net2 = Network(shape, bias=0)
        net1.randomise([0.1, 1.4])
        net2.randomise([0.1, 0.4])

        f1 = open('/tmp/net1.pickle', 'w')
        f2 = open('/tmp/net2.pickle', 'w')
        #print net1.weights
        #print net1.__reduce__()

        cPickle.dump(net1, f1)
        cPickle.dump(net2, f2)
        f1.close()
        f2.close()
        f3 = open('/tmp/net1.pickle')
        f4 = open('/tmp/net2.pickle')
        net3 = cPickle.load(f3)
        net4 = cPickle.load(f4)
        inputs = range(shape[0])
        o1, o2, o3, o4 = net1.opinion(inputs), net2.opinion(inputs), net3.opinion(inputs), net4.opinion(inputs)
        if o1 != o3:
            print o1, o3
        if o2 != o4:
            print o2, o4



def learn_generic_genetic():
    net = Network([2,3,1], bias=1)
    net.randomise([-0.5, 0.5], 1)

    set = [([0,0],[0]), ([1,0],[1]), ([0,1],[1]), ([1,1],[0]), ]

    def evaluator(net):
        s = 0
        for i, o in set:
            s += (net.opinion(i)[0] - o[0]) ** 2
        #print s
        return int(s * 100000)

    r = net.generic_genetic(evaluator, 1000, 20)
    print r
    for i, o in set:
        print "%s -> %04f %s  %s" % (i, net.opinion(i)[0], o, abs(net.opinion(i)[0] - o[0]))



def test_set_weights():
    net = Network([2,3,1])
    net.randomise([0.0, 0.0]) #zero weights
    set = [[0,0], [1,0], [0,1], [1,1]]
        
    #or x in set:
    #   print x, net.opinion(x)

    #setting to approximately these weights, found by backprop for xor
    #(6.7077757974206165, -3.7077785238532299, -3.9139562896228575,
    #7.1901917938926632, 5.2392210854276717, 5.4786265402520167,
    #-9.7690926723158142, -9.745254373178156, 13.76096653909681,
    #3.6491208624624705e+233)

    net.set_single_weight(0,0,0,6.7)
    net.set_single_weight(0,1,0,-3.7)
    net.set_single_weight(0,0,1,-3.9)    
    net.set_single_weight(0,1,1,7.2)
    net.set_single_weight(0,0,2,5.2)
    net.set_single_weight(0,1,2,5.5)    

    net.set_single_weight(1,0,0,-9.8)
    net.set_single_weight(1,1,0,-9.7)
    net.set_single_weight(1,2,0,13.8)

        
    for x in set:
        print x, net.opinion(x)
        

    #setting to adhoc approximation of xor

    net.set_single_weight(0,0,0, 4)
    net.set_single_weight(0,1,0,-2)
    
    net.set_single_weight(0,0,1,-2)    
    net.set_single_weight(0,1,1, 4)

    net.set_single_weight(0,0,2, 2)
    net.set_single_weight(0,1,2, 2)    

    net.set_single_weight(1,0,0,-10)
    net.set_single_weight(1,1,0,-10)
    net.set_single_weight(1,2,0, 15)

        
    for x in set:
        print x, net.opinion(x)
    



if __name__ == '__main__':
    #test_opinion_strings()
    #test_opinion_arrays()
    #learn_best_of_set(arrays=0)
    #learn_best_of_set(arrays=1)
    #test_arrays()
    #learn_generic_genetic()
    test_set_weights()
    #n = learn_xor('backprop')
    #s = n.weights
    #import struct
    #w = struct.unpack('10d', s)
    #print w

        
    #test_pickle()
    if 1:
        learn_count()
        test_save(learn_count)
        learn_xor_neg('anneal')
        learn_xor_neg('backprop')
        learn_xor('anneal')
        n = learn_xor('backprop')
        print n.shape
        print dir(n)
        print len(n.weights)
        learn_symmetry('anneal')
        learn_symmetry('both')
        test_weight_strings(learn_count)
    print "ending!"
