
#include <stdlib.h>
#include <RMQ.hpp>
#include <string.h>
#include "esa.h"

lcp_inter_t get_match_from( const esa_t *C, const char *query, size_t qlen, saidx_t k, lcp_inter_t ij);
static lcp_inter_t *get_interval( const esa_t *C, lcp_inter_t *ij, char a);
static void esa_init_cache_dfs( esa_t *C, char *str, size_t pos, const lcp_inter_t *in);
void esa_init_cache_fill( esa_t *C, char *str, size_t pos, const lcp_inter_t *in);

static int esa_init_SA( esa_t *c);
static int esa_init_LCP( esa_t *c);


/** @brief Initializes an ESA.
 *
 * This function initializes an ESA with respect to the provided sequence.
 * @param C - The ESA to initialize.
 * @param S - The sequence
 * @returns 0 iff successful
 */
int esa_init( esa_t *C, const char *S, size_t length){
	C->S = S;
	C->SA = NULL;
	C->ISA = NULL;
	C->LCP = NULL;
	C->len = 0;
	C->FVC = NULL;

	int result;

	C->len = length;

	if( C->S == NULL ) return 1;

	result = esa_init_SA(C);
	if(result) return result;


	result = esa_init_LCP(C);
	if(result) return result;

	C->rmq_lcp = new RMQ_n_1_improved(C->LCP, C->len);

	return 0;
}

/** @brief Free the private data of an ESA. */
void esa_free( esa_t *C){
	delete C->rmq_lcp;
	free( C->SA);
	free( C->ISA);
	free( C->LCP);
}

void all_matches(esa_t *C, const std::string& Q){
	auto qlen = Q.size();
	auto T = Q.data();

	auto j = 0;
	while( j < qlen){
		auto ij = get_match(C, T+j, qlen - j);
		if (ij.l < 0) ij.l = 0;
		j += ij.l + 1;
	}

}

/**
 * Computes the SA given a string S. To do so it uses libdivsufsort.
 * @param C The enhanced suffix array to use. Reads C->S, fills C->SA.
 * @returns 0 iff successful
 */
int esa_init_SA(esa_t *C){
	// assert c.S
	if( !C || C->S == NULL){
		return 1;
	}
	if( C->SA == NULL){
		C->SA = (saidx_t*) malloc(C->len * sizeof(saidx_t));
		if( C->SA == NULL){
			return 2;
		}
	}
	
	saidx_t result;
	result = divsufsort((const unsigned char*)C->S, C->SA, C->len);
	
	return result;
}

/**
 * This function implements an alternative way of computing an LCP
 * array for a given suffix array. It uses an intermediate `phi`
 * array, hence the name. It's a bit faster than the other version.
 * @param C The enhanced suffix array to compute the LCP from.
 * @returns 0 iff successful
 */
int esa_init_LCP( esa_t *C){
	const char *S = C->S;
	saidx_t *SA  = C->SA;
	saidx_t len  = C->len;
	
	// Trivial safety checks
	if( !C || S == NULL || SA == NULL || len == 0){
		return 1;
	}
	
	// Allocate new memory
	if( C->LCP == NULL){
		// The LCP array is one element longer than S.
		C->LCP = (saidx_t*) malloc((len+1)*sizeof(saidx_t));
		if( C->LCP == NULL ){
			return 3;
		}
	}
	saidx_t *LCP = C->LCP;
	
	LCP[0] = -1;
	LCP[len] = -1;
	
	// Allocate temporary arrays
	saidx_t *PHI = (saidx_t *) malloc( len * sizeof(saidx_t));
	saidx_t *PLCP = (saidx_t *) malloc( len * sizeof(saidx_t));
	if( !PHI || !PLCP){
		free(PHI);
		free(PLCP);
		return 2;
	}
	
	PHI[SA[0]] = -1;
	ssize_t i, k;
	
	
	for( i=1; i< len; i++){
		PHI[SA[i]] = SA[ i-1];
	}
	
	ssize_t l = 0;
	for( i = 0; i< len ; i++){
		k = PHI[i];
		if( k != -1 ){
			while( k+l < len && S[k+l] == S[i+l]){
				l++;
			}
			PLCP[i] = l;
			l--;
			if( l < 0) l = 0;
		} else {
			PLCP[i] = -1;
		}
	}
	
	// unpermutate the LCP array
	for( i=1; i< len; i++){
		LCP[i] = PLCP[SA[i]];
	}
	
	free(PHI);
	free(PLCP);
	return 0;
}

/**
 * Given the LCP interval for a string `w` this function calculates the 
 * LCP interval for `wa` where `a` is a single character.
 * The empty interval is represented by ij.i == -1 and ij.j == -1.
 * @param C This is the enhanced suffix array of the subject sequence.
 * @param ij The prefix `w` is given implicitly by the ij LCP interval.
 * @param a The next character in the query sequence.
 * @returns A reference to the new LCP interval.
 */
static lcp_inter_t *get_interval( const esa_t *C, lcp_inter_t *ij, char a){
	saidx_t i = ij->i;
	saidx_t j = ij->j;


	const saidx_t *SA = C->SA;
	const saidx_t *LCP = C->LCP;
	const char *S = C->S;
	RMQ *rmq_lcp = C->rmq_lcp;
	
	// check for singleton or empty interval
	if( i == j ){
		if( S[SA[i]] == a){
			ij->i = ij->j = i;
		} else {
			ij->i = ij->j = -1;
		}
		return ij;
	}

	saidx_t l, m;
	
	m = ij->m; // m is now any minimum in (i..j]
	l = ij->l;
	
	/* We now use an abstract binary search. Think of `i` as the
	 * lower and `j` as the upper boundary. Then `m` is the new
	 * middle. */
	do {
		// check if `m` will be a new lower or upper bound.
		if( S[ SA[m] + l] <= a ){
			i = m;
		} else {
			j = m - 1;
		}
		
		if( i == j ){
			break; // `a` not found, exit early
		}
		
		// find the next LCP boundary
		m = rmq_lcp->query(i+1, j);
	} while( LCP[m] == l);

	// final sanity check
	if( S[SA[i] + l] == a){
		ij->i = i;
		ij->j = j;
		/* Also return the length of the LCP interval including `a` and
		 * possibly even more characters. Note: l + 1 <= LCP[m] */
		ij->l = LCP[m];
		ij->m = m;
	} else {
		ij->i = ij->j = -1;
	}
	
	return ij;
}

/**
 * This function computes the LCPInterval for the longest prefix of `query` which
 * can be found in the subject sequence. Compare Ohlebusch get_interval Alg 5.2 p.119
 * @param C The enhanced suffix array for the subject.
 * @param query The query sequence.
 * @param qlen The length of the query. Should correspond to `strlen(query)`.
 * @returns The LCP interval for the longest prefix.
 */
lcp_inter_t get_match( const esa_t *C, const char *query, size_t qlen){
	lcp_inter_t res = {0,0,0,0};

	// sanity checks
	if( !C || !query || !C->len || !C->SA || !C->LCP || !C->S || !C->rmq_lcp ){
		res.i = res.j = res.l = -1;
		return res;
	}
	
	lcp_inter_t ij = { 0, 0, C->len-1, 0};
	
	// TODO: This should be cached in a future version
	ij.m = C->rmq_lcp->query(1,C->len-1);

	return get_match_from(C, query, qlen, 0, ij);
}


/** @brief Compute the LCP interval of a query from a certain starting interval.
 *
 * @param C - The enhanced suffix array for the subject.
 * @param query - The query sequence.
 * @param qlen - The length of the query. Should correspond to `strlen(query)`.
 * @param k - The starting index into the query.
 * @param ij - The LCP interval for the string `query[0..k]`.
 * @returns The LCP interval for the longest prefix.
 */
lcp_inter_t get_match_from( const esa_t *C, const char *query, size_t qlen, saidx_t k, lcp_inter_t ij){

	if( ij.i == -1 && ij.j == -1){
		return ij;
	}

	// fail early on singleton intervals.
	if( ij.i == ij.j){

		// try to extend the match. See line 513 below.
		saidx_t p = C->SA[ij.i];
		size_t k = ij.l;
		const char *S = (const char *)C->S;

		for( ; k< qlen && S[p+k]; k++ ){
			if( S[p+k] != query[k]){
				ij.l = k;
				return ij;
			}
		}

		ij.l = k;
		return ij;
	}

	saidx_t l, i, j, p;
	saidx_t m = qlen;

	lcp_inter_t res = ij;
	
	saidx_t *SA = C->SA;
	const char *S = (const char *)C->S;
	
	// Loop over the query until a mismatch is found
	do {
		get_interval( C, &ij, query[k]);
		i = ij.i;
		j = ij.j;
		
		// If our match cannot be extended further, return.
		if( i == -1 && j == -1 ){
			res.l = k;
			return res;
		}
		
		res.i = ij.i;
		res.j = ij.j;

		l = m;
		if( i < j){
			/* Instead of making another RMQ we can use the LCP interval calculated
			 * in get_interval */
			if( ij.l < l ){
				l = ij.l;
			}
		}
		
		// Extend the match
		for( p = SA[i]; k < l && S[p+k] && query[k]; k++){
			if( S[p+k] != query[k] ){
				res.l = k;
				return res;
			}
		}
		
		// TODO: Verify if this is the best solution
		// You shall not pass the null byte.
		if( k < l && (!S[p+k] || !query[k])){
			res.l = k;
			return res;
		}

		k = l;
	} while ( k < m);

	res.l = m;
	return res;
}

