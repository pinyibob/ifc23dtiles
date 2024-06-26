
#ifndef MD5_H
#define MD5_H

/* Parameters of MD5. */
#define s11 7
#define s12 12
#define s13 17
#define s14 22
#define s21 5
#define s22 9
#define s23 14
#define s24 20
#define s31 4
#define s32 11
#define s33 16
#define s34 23
#define s41 6
#define s42 10
#define s43 15
#define s44 21

/**
 * @Basic MD5 functions.
 *
 * @param there uint32_t.
 *
 * @return one uint32_t.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

 /**
  * @Rotate Left.
  *
  * @param {num} the raw number.
  *
  * @param {n} rotate left n.
  *
  * @return the number after rotated left.
  */
#define ROTATELEFT(num, n) (((num) << (n)) | ((num) >> (32-(n))))

  /**
   * @Transformations for rounds 1, 2, 3, and 4.
   */
#define FF(a, b, c, d, x, s, ac) { \
  (a) += F ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) { \
  (a) += G ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
  (a) += H ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
  (a) += I ((b), (c), (d)) + (x) + ac; \
  (a) = ROTATELEFT ((a), (s)); \
  (a) += (b); \
}

#include <string>
#include <cstring>
#include <stdint.h>

using std::string;

class MD5 {
public:
	/* Construct a MD5 object with a string. */
	MD5(const string& message);

	/* Construct a MD5 object with a uchar araay. */
	MD5(const uint8_t* data, const int length);

	/* Generate md5 digest. */
	const uint8_t* getDigest();

	/* Convert digest to string value */
	string toStr();

private:
	/* Initialization the md5 object, processing another message block,
	 * and updating the context.*/
	void init(const uint8_t* input, size_t len);

	/* MD5 basic transformation. Transforms state based on block. */
	void transform(const uint8_t block[64]);

	/* Encodes input (usigned long) into output (uint8_t). */
	void encode(const uint32_t* input, uint8_t* output, size_t length);

	/* Decodes input (uint8_t) into output (usigned long). */
	void decode(const uint8_t* input, uint32_t* output, size_t length);

private:
	/* Flag for mark whether calculate finished. */
	bool finished;

	/* state (ABCD). */
	uint32_t state[4];

	/* number of bits, low-order word first. */
	uint32_t count[2];

	/* input buffer. */
	uint8_t buffer[64];

	/* message digest. */
	uint8_t digest[16];

	/* padding for calculate. */
	static const uint8_t PADDING[64];

	/* Hex numbers. */
	static const char HEX_NUMBERS[16];
};

#endif // MD5_H