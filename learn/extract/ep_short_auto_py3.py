
#http://stackoverflow.com/questions/31324739/finding-gradient-of-a-caffe-conv-filter-with-regards-to-input
#import os
#import numpy as np
#import caffe
#import sys

#from google.protobuf import text_format
import caffe
#from caffe.proto import caffe_pb2
import struct
import sys
import time

args = sys.argv
print(args[1])
#sys.exit()

caffe.set_mode_cpu()
#net = caffe.Net("/home/yss/shogi/learn/aoba_zero_256x40b.prototxt",args[1],caffe.TEST);
#net = caffe.Net("/home/yss/shogi/learn/aoba_zero_256x20b_mb128.prototxt",args[1],caffe.TEST)
#net = caffe.Net("/home/yss/shogi/learn/aoba_zero_192x10b_mb128_p2187.prototxt",args[1],caffe.TEST)
#net = caffe.Net("/home/yss/shogi/learn/aoba_zero_64x10b_mb128_p2187.prototxt",args[1],caffe.TEST)
net = caffe.Net("/home/yss/shogi/learn/aoba_zero_256x20b_mb128_p2187.prototxt",args[1],caffe.TEST)

#time.sleep(30)

#net = caffe_pb2.NetParameter()
#text_format.Merge(open("aya_m3_solver.prototxt").read(), net)


#solver = caffe.SGDSolver( "aya_m3_solver.prototxt" )
#net = solver.net

#print net.blobs
#print net.inputs  # >> ['data']
#print net.blobs['data']
#print [(k, v.data.shape) for k, v in net.blobs.items()]

#You will see a dictionary storing a "caffe blob" object for each layer in the net. Each blob has storing room for both data and gradient

#print net.blobs['data'].data.shape    # >> (1, 49, 19, 19)
#print net.blobs['data'].diff.shape    # >> (1, 49, 19, 19)

#And for a convolutional layer:

#print net.blobs['conv1'].data.shape    # >> (1, 256, 19, 19)
#print net.blobs['conv2'].diff.shape    # >> (1, 256, 19, 19)

#print net.params['conv1_5x5_256'][0].data.shape  # >> (256, 49, 5, 5)
#print net.params['conv1_5x5_256'][0].data.shape[0] # >> 256
#print net.params['conv1_5x5_256'][0].data.shape[1] # >> 49
#print net.params['conv1_5x5_256'][0].data.shape[2] # >> 5
#print net.params['conv1_5x5_256'][0].data.shape[3] # >> 5


def short_str(s):
	#r = '%.6g' % s
	r = '%.3g' % s     # LZ style. this is maybe ok.
	u = r
	if ( r[0:2]== '0.' ) :
		u = r[1:]
	if ( r[0:3]=='-0.' ) :
		u = '-' + r[2:]
	return u
#    return '{0:.3f}'.format(s)
#    return '{0:.3f}'.format(s)

#bf = open('binary.bin', 'wb')
bf = open('binary.txt', 'w')
sum = 0
fc_sum = 0
cv_sum = 0

bf.write('2\n')    # version

n_layer = len( net.params.items() )
print(n_layer)
#print net.params.items()[0][0]
#print net.params.items()[1]
#print net.params.items()[2]


# multi line comment from """ to """
"""
for loop in range(n_layer):
	name = net.params.items()[loop][0]
	print loop, name
	print net.params[name][0].data.shape
	a0 = net.params[name][0].data.shape[0]
	#print a0
	if 'conv' in name:
		a1 = net.params[name][0].data.shape[1]
	#if 'fc' in name:
	if ('fc' in name or 'ip' in name):
		b0 = net.params[name][1].data.shape[0]
		print b0
	if 'bn' in name:
		#print net.params[name][0].data.shape[0]
		#print net.params[name][0].data.shape[1]
		a1 = net.params[name][1].data.shape[0]
		b0 = net.params[name][2].data.shape[0]
		print loop , name, a0,a1, ":", b0
		#print net.params[name][3].data.shape[0]
		for i in range(a0):
			d = net.params[name][0].data[i]
			print i,d
		for i in range(a1):
			d = net.params[name][1].data[i]
			print i,d
		d = net.params[name][2].data[0]
		print d
sys.exit()
"""

for loop in range(n_layer):
	name = list(net.params.items())[loop][0]
	#name = net.params.items()[loop][0]
	#print loop , name
	a0 = net.params[name][0].data.shape[0]
	#print a0
	ct = 0;
	if 'bn' in name:
		a1 = net.params[name][1].data.shape[0]
		b0 = net.params[name][2].data.shape[0]
		print(loop , name, a0,a1, ":", b0)

		for i in range(2):
			ct = 0
			for j in range(a0):
				d = net.params[name][i].data[j]
				#bf.write(struct.pack("f", d))
				if ct==1: bf.write(' ')
				ct = 1
				bf.write(short_str(d))
				#bf.write(str(d))
				sum += 1
			bf.write('\n')
		# this is alwasys "999.982" ? -> needed! scale_factor
		d = net.params[name][2].data[0]
		print("bn_scale_factor=", d)
		#bf.write(struct.pack("f", d))
		#bf.write(str(d))
		#sum += 1
		#bf.write('\n')
		continue

	a1 = net.params[name][0].data.shape[1]
	if ('fc' in name or 'ip' in name):
		b0 = net.params[name][1].data.shape[0]
		print(loop , name, a0,a1, ":", b0)
		for i in range(a0):
			for j in range(a1):
				d = net.params[name][0].data[i][j]
				#bf.write(struct.pack("f", d))
				if ct==1: bf.write(' ')
				ct = 1
				#bf.write(str(d))
				bf.write(short_str(d))
				sum += 1
				fc_sum += 1
		bf.write('\n')
	else:
		a2 = net.params[name][0].data.shape[2]
		a3 = net.params[name][0].data.shape[3]
		b0 = net.params[name][1].data.shape[0]
		print(loop , name, a0,a1,a2,a3, ":", b0)

		for i in range(a0):
			for j in range(a1):
				for k in range(a2):
					for m in range(a3):
						d = net.params[name][0].data[i][j][k][m]
						#bf.write(struct.pack("f", d))
						if ct==1: bf.write(' ')
						ct = 1
						#bf.write(str(d))
						bf.write(short_str(d))
						sum += 1
						cv_sum += 1
		bf.write('\n')

	ct = 0
	for i in range(b0):
		d = net.params[name][1].data[i]
		#bf.write(struct.pack("f", d))
		if ct==1: bf.write(' ')
		ct = 1
		#bf.write(str(d))
		bf.write(short_str(d))
		sum += 1
	bf.write('\n')

bf.close()
print("convert done...", sum, " (fc_sum=", fc_sum, " cv_sum=", cv_sum, ")")
sys.exit()

"""
#for v in net.params.items()
#	print [(v[0].data.shape, v[1].data.shape)]

#print net.params[v][0].data.shape  # >> (256, 256, 3, 3)
#print net.params['conv2_3x3_256'][0].data.shape  # >> (256, 256, 3, 3)
#print net.params['conv11_3x3_256'][0].data.shape # >> (256, 256, 3, 3)
#print net.params['conv12_3x3_1_0'][0].data.shape # >> (1, 256, 3, 3)
#print [(k, v[0].data.shape, v[1].data.shape) for k, v in net.params.items()]

#print net.layers
#print net.layers[0].blobs
#print net.layers[1].blobs
#print len(net.layers[1].blobs)  # >> 0
#print net.layers[1].blobs[0].data.shape # Err

print len(net.blobs['data'].data[0])  # >> 49
#print net.blobs['data'].data[0]
print len(net.params['conv1_5x5_256'][0].data)  # >> 256,
print net.params['conv1_5x5_256'][0].data[0][0][0][0]
#print net.params['conv1_5x5_256'][0].data       # contains the weight parameters, an array of shape (256, 1, 5, 5)
print len(net.params['conv1_5x5_256'][1].data)	# >> 256
#print net.params['conv1_5x5_256'][1].data       # bias, 256float, contains the bias parameters, an array of shape (256,)
#print net.params['conv1_5x5_256'][2].data # Err
print len(net.params['conv2_3x3_256'][0].data)  # >> 256
print len(net.params['conv2_3x3_256'][1].data)  # >> 256

print 'conv12_3x3_1_0'
print len(net.params['conv12_3x3_1_0'][0].data)  # >> 1
#print net.params['conv12_3x3_1_0'][0].data
print len(net.params['conv12_3x3_1_0'][1].data)  # >> 1
#print net.params['conv12_3x3_1_0'][1].data
#print net.params['flat0'][0].data # Err
#print net.params['softmax0'][0].data # Err

sys.exit()

bf = open('binary.bin', 'wb')
sum = 0
for i in range(256):
	for j in range(49):
		for k in range(5):
			for m in range(5):
				d = net.params['conv1_5x5_256'][0].data[i][j][k][m]
				bf.write(struct.pack("f", d))
				sum += 1

bf.close()
print sum
print 'done'
"""