from scipy.misc import imread
import numpy as np
from numpy import linalg as LA

car = imread("assets/car.ppm")
car2 = imread("assets/car2.ppm")
tux = imread("assets/tux.ppm")

car_size = car.shape[0]*car.shape[1]
car2_size = car2.shape[0]*car2.shape[1]
tux_size = tux.shape[0]*tux.shape[1]

car = car.reshape(car_size, 3)
car2 = car2.reshape(car2_size, 3)
tux = tux.reshape(tux_size, 3)

car_vigenere = imread("assets/car_vigenere.ppm").reshape(car_size, 3)
car2_vigenere = imread("assets/car2_vigenere.ppm").reshape(car2_size, 3)
tux_vigenere = imread("assets/tux_vigenere.ppm").reshape(tux_size, 3)

car_affine = imread("assets/car_affine.ppm").reshape(car_size, 3)
car2_affine = imread("assets/car2_affine.ppm").reshape(car2_size, 3)
tux_affine = imread("assets/tux_affine.ppm").reshape(tux_size, 3)

car_aes_ecb = imread("assets/car_aes_ecb.ppm").reshape(car_size, 3)
car2_aes_ecb = imread("assets/car2_aes_ecb.ppm").reshape(car2_size, 3)
tux_aes_ecb = imread("assets/tux_aes_ecb.ppm").reshape(tux_size, 3)

car_aes_cbc = imread("assets/car_aes_cbc.ppm").reshape(car_size, 3)
car2_aes_cbc = imread("assets/car2_aes_cbc.ppm").reshape(car2_size, 3)
tux_aes_cbc = imread("assets/tux_aes_cbc.ppm").reshape(tux_size, 3)

car_orig_norm = LA.norm(car, axis=1)
car2_orig_norm = LA.norm(car2, axis=1)
tux_orig_norm = LA.norm(tux, axis=1)

car_vigenere_dist = car_orig_norm-LA.norm(car_vigenere, axis=1)
car2_vigenere_dist = car2_orig_norm-LA.norm(car2_vigenere, axis=1)
tux_vigenere_dist = tux_orig_norm-LA.norm(tux_vigenere, axis=1)

car_affine_dist = car_orig_norm-LA.norm(car_affine, axis=1)
car2_affine_dist = car2_orig_norm-LA.norm(car2_affine, axis=1)
tux_affine_dist = tux_orig_norm-LA.norm(tux_affine, axis=1)

car_aes_ecb_dist = car_orig_norm-LA.norm(car_aes_ecb, axis=1)
car2_aes_ecb_dist = car2_orig_norm-LA.norm(car2_aes_ecb, axis=1)
tux_aes_ecb_dist = tux_orig_norm-LA.norm(tux_aes_ecb, axis=1)

car_aes_cbc_dist = car_orig_norm-LA.norm(car_aes_cbc, axis=1)
car2_aes_cbc_dist = car2_orig_norm-LA.norm(car2_aes_cbc, axis=1)
tux_aes_cbc_dist = tux_orig_norm-LA.norm(tux_aes_cbc, axis=1)

vigenere_dist_all = np.concatenate([car_vigenere_dist, car2_vigenere_dist, tux_vigenere_dist])
affine_dist_all = np.concatenate([car_affine_dist, car2_affine_dist, tux_affine_dist])
aes_ecb_dist_all = np.concatenate([car_aes_ecb_dist, car2_aes_ecb_dist, tux_aes_ecb_dist])
aes_cbc_dist_all = np.concatenate([car_aes_cbc_dist, car2_aes_cbc_dist, tux_aes_cbc_dist])

vigenere_mean = np.mean(vigenere_dist_all)
affine_mean = np.mean(affine_dist_all)
aes_ecb_mean = np.mean(aes_ecb_dist_all)
aes_cbc_mean = np.mean(aes_cbc_dist_all)

vigenere_std = np.std(vigenere_dist_all)
affine_std = np.std(affine_dist_all)
aes_ecb_std = np.std(aes_ecb_dist_all)
aes_cbc_std = np.std(aes_cbc_dist_all)

#vigenere_corr = np.correlate(vigenere_dist_all, vigenere_dist_all)
#affine_corr = np.correlate(affine_dist_all, affine_dist_all)
#aes_ecb_corr = np.correlate(aes_ecb_dist_all, aes_ecb_dist_all)
#aes_cbc_corr = np.correlate(aes_cbc_dist_all, aes_cbc_dist_all)


print "Vigenere mean distance:", vigenere_mean
print "Affine mean distance:", affine_mean
print "AES ECB mean distance:", aes_ecb_mean
print "AES CBC mean distance:", aes_cbc_mean

print "Vigenere std of distances:", vigenere_std
print "Affine std of distances:", affine_std
print "AES ECB std of distances:", aes_ecb_std
print "AES CBC std of distances:", aes_cbc_std

#print "Vigenere correlation of distances:", vigenere_corr
#print "Affine correlation of distances:", affine_corr
#print "AES ECB correlation of distances:", aes_ecb_corr
#print "AES CBC correlation of distances:", aes_cbc_corr