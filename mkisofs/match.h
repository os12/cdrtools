/*
 * 27th March 1996. Added by Jan-Piet Mens for matching regular expressions
 * 		    in paths.
 * 
 */

/*
 * 	$Id: match.h,v 1.2 1999/03/02 03:41:25 eric Exp $
 */

#include "fnmatch.h"

int add_match	__PR((char *fn));
int matches	__PR((char *fn));

int i_add_match __PR((char *fn));
int i_matches	__PR((char *fn));
int i_ishidden	__PR((void));

int j_add_match __PR((char *fn));
int j_matches	__PR((char *fn));
int j_ishidden	__PR((void));

void add_list	__PR((char *fn));
void i_add_list __PR((char *fn));
void j_add_list __PR((char *fn));
