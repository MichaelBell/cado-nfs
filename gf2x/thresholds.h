#ifndef THRESHOLDS_H_
#define THRESHOLDS_H_

/* This file contains sensible defaults to start with, which are updated
 * by the tuning program */

/* If you read "placeholder" here, it means that the tuning program has
 * not been run (or has not completed) */
#define	TOOM_TUNING_INFO		"placeholder"
#define FFT_TUNING_INFO		"placeholder"

/* First size for which KARA is used. Essentially hard-coded, since the
 * sizes up to 9 words are already karatsuba, unrolled. The unrolled
 * routines handle their own temporary storage on the stack.
 */
#define MUL_KARA_THRESHOLD	10

/* First size for which TC3W is used. It is assumed that TC3W is used
 * before TC3 kicks in. */
/* must be >= 8 */
#define MUL_TOOMW_THRESHOLD		18

/* First size for which TC3 is used. This threshold is informative, it is
 * not used in the code */
/* must be >= 17 */
#define MUL_TOOM_THRESHOLD		21

/* First size for which TC4 is used. This threshold is informative, it is
 * not used in the code */
/* must be >= 30 */
#define MUL_TOOM4_THRESHOLD		235

/* Size above which TC4 is the only TC variant used. */
#define MUL_TOOM4_ALWAYS_THRESHOLD		1600

/* First size for which TC3U is used */
/* must be >= 33 */
#define MUL_TOOMU_THRESHOLD		49

/* Size above which TC3U is the only TCU variant used. */
#define MUL_TOOMU_ALWAYS_THRESHOLD		2049

/* The default values here are appropriate for the tuning program.
 * Appropriate values are substituted later on. Note that the tuning
 * table is always created with this size, to that re-tuning is possible
 * at any time. */
#define	TOOM_TUNING_LIMIT	2048
#define BEST_TOOM_TABLE {}

#define	BEST_UTOOM_TABLE {}

/* This macro is not what you think, and does not completely belong here.
 * It merely recalls that the FFT code *DOES NOT WORK* below this size.
 * So MUL_FFT_TABLE should not wander in this bleak range.
 */
#define MUL_FFT_THRESHOLD 28

/* {n, K} means use FFT(|K|) up from n words, */
/* where |K|<3 stands for Toom-Cook 3, K < 0 means use FFT2 */
#undef MUL_FFT_TABLE

/* These flags are for internal use */
#define	GF2X_SELECT_KARA	0	/* do not change ! */
#define	GF2X_SELECT_TC3		1
#define	GF2X_SELECT_TC3W	2
#define	GF2X_SELECT_TC4		3
#define	GF2X_SELECT_UNB_DFLT	0
#define	GF2X_SELECT_UNB_TC3U	1	/* do not change ! */

#endif	/* THRESHOLDS_H_ */
