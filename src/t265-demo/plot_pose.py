import numpy as np 
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

plt.close('all')


X = np.loadtxt("pose.txt", skiprows=1, usecols=(1,2,3,4,5,6,7), delimiter=",")
#print(X)
print(X.shape)

# plot 3D
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')
ax.scatter(X[:,0],X[:,1],X[:,2])
plt.xlabel("X")
plt.ylabel("Y")
plt.title("position 3D")
plt.savefig("pose3D")
#plt.show()


# plot 2D
plt.figure()
plt.plot(X[:,0],X[:,1],'o')
plt.xlabel("X")
plt.ylabel("Y")
plt.axis('equal')
plt.title("projection X/Y")
plt.savefig("poseXY")

plt.figure()
plt.plot(X[:,0],X[:,2],'o')
plt.xlabel("X")
plt.ylabel("Z")
plt.axis('equal')
plt.title("projection X/Z")
plt.savefig("poseXZ")

plt.figure()
plt.plot(-X[:,0],X[:,2],'o')
plt.xlabel("-X")
plt.ylabel("Z")
plt.axis('equal')
plt.title("projection -X/Z")
plt.savefig("pose-XZ")

plt.figure()
plt.plot(X[:,1],X[:,2],'o')
plt.xlabel("Y")
plt.ylabel("Z")
plt.axis('equal')
plt.title("projection YZ")
plt.savefig("poseYZ")


# plot axes
plt.figure()
plt.plot(X[:,0],'.')
plt.xlabel("k")
plt.ylabel("X")
plt.title("x")
plt.savefig("poseX")

plt.figure()
plt.plot(X[:,1],'.')
plt.xlabel("k")
plt.ylabel("Y")
plt.title("y")
plt.savefig("poseY")

plt.figure()
plt.plot(X[:,2],'.')
plt.xlabel("k")
plt.ylabel("Z")
plt.title("z")
plt.savefig("poseZ")

plt.show()


# export as tum
N = X.shape[0]
k = np.arange(N)
K = k.reshape(N,1)
XX = np.hstack((K,X))

np.savetxt("pose.tum", XX, delimiter=" ")