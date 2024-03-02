#include "libword.h"

#define MLDEV_PORT 83


#define COPENI	1	/* OPEN for input
			   arg1:	Device
			   arg2:	FN1
			   arg3:	FN2
			   arg4:	SNAME
			   arg5:	OPEN mode */

#define COPENO	2	/* OPEN for output
			   arg1:	Device
			   arg2:	FN1
			   arg3:	FN2
			   arg4:	SNAME
			   arg5:	OPEN mode */

#define CDATA	3	/* Write data (Word count includes data words and
			   byte count word)
			   arg1:	# of bytes of data
			   args:	Data, packed ILDB-style into at most
					200 words */

#define CALLOC	4	/* Read allocate
			   Allocation in bytes */

#define CICLOS	5	/* Input CLOSE
			   Ignored argument */

#define COCLOS	6	/* Output CLOSE
			   Ignored argument */

#define CFDELE	7	/* DELETE or RENAME
			   arg1:	Device
			   arg2:	FN1
			   arg3:	FN2
			   arg4:	0 or new FN1
			   arg5:	0 or new FN2
			   arg6:	SNAME */

#define CRNMWO	010	/* RENAME while open
			   arg1:	new FN1
			   arg2:	new FN2 */

#define CNOOP	011	/* No-op
			   arg:	Echoed by the no-op reply */

#define CACCES	12	/* ACCESS
			   arg1:	Transaction number
			   arg2:	Access pointer
			   arg3:	New allocation */

#define CSYSCL	13	/* Random system call.  11 (decimal) arguments
			   arg1:	System call name in sixbit
			   arg2:	Flags (control-bit argument)
			   arg3:	Number of following arguments that
					have significance
			   arg4:	1st argument to system call
			   ...
			   arg11:	8th argument to system call */

#define CREUSE	14	/* Reinitialize yourself, because we are about to
			   be reused */

#define RDATA	1	/* Input data (word count includes data words and
			   byte count word)
			   arg1:	# of bytes of data
			   args:	Data, packed ILDB-style into words.
			   Note:  Slave must always send fully populated
			   words except when EOF is reached. */

#define ROPENI	2	/* Input OPEN status; 1 or 11 arguments
			   arg1: -1 => succeeded >0 => OPEN loss number
			   arg2-arg11 are present only if arg1 is -1
			   arg2:	Real device name (usually SIXBIT/DSK/)
			   arg3:	 Real FN1
			   arg4:	Real FN2
			   arg5:	Real SNAME
			   arg6:	File length
			   arg7:	Byte size open in
			   arg8:	File length in byte size written in
			   arg9:	Byte size written in
			   arg10:	FILDTP - -1 if RFDATE wins on this file
			   arg11:	Creation data (if RFDATE wins) */

#define ROPENO	3	/* Output OPEN status.  1 or 11 arguments
			   arg1:	-1 => succeeded >0 => OPEN loss number
			   arg2-arg11 present only if arg1 is -1
			   arg2:	Real device name
			   arg3:	Real FN1
			   arg4:	Real FN2
			   arg5:	Real SNAME
			   arg6:	File length (-1 => FILLEN fails on
					this device)
			   arg7:	Byte size open in
			   arg8:	File length in byte size written in
			   arg9:	Byte size written in
			   arg10:	FILDTP - -1 if RFDATE wins on this file
			   arg11:	Creation data (if RFDATE wins) */

#define REOF	4	/* EOF on input
			   Ignored argument */

#define RFDELE	5	/* DELETE/RENAME status.
			   1st argument:
			   RH:	-1 => succeeded
				0 => failure code
			   LH:	-1 => DELETE or RENAME
				0 => RENAME while open
			   If RENAME while open call succeeded, there are
			   4 more arguments:
			   arg2:	Real device name
			   arg3:	Real FN1 (as renamed)
			   arg4:	Real FN2
			   arg5:	Real SNAME */

#define RNOOP	6	/* No-op reply
			   arg: Echo argument send by no-op command */

#define RACCES	7	/* Acknowledge access
			   arg:	Transaction number from CACCES */

#define RSYSCL	010	/* Respond to system call.  9 arguments
			   arg1:	<error code or 0>,,<# of times call
					skipped>
			   arg2:	1st value from system call
			   ...
			   arg9:	8th value from system call */

#define RICLOS	011	/* Respond to input CLOSE */

#define ROCLOS	012	/* Respond to output CLOSE */

#define RIOC	013	/* Reflect an IOC error
			   arg:	IOC error code, right-justified in word */

#define RREUSE	014	/* Acknowledge a CREUSE
			   Ignored argument */

#define ENSDV 001	/* NO SUCH DEVICE */
#define ENSIO 002	/* WRONG DIRECTION */
#define ETMTR 003	/* TOO MANY TRANSLATIONS */
#define ENSFL 004	/* FILE NOT FOUND */
#define EFLDR 005	/* DIRECTORY FULL */
#define EFLDV 006	/* DEVICE FULL */
#define ENRDV 007	/* DEVICE NOT READY */
#define ENADV 010	/* DEVICE NOT AVAILABLE */
#define EBDFN 011	/* ILLEGAL FILE NAME */
#define ENSMD 012	/* MODE NOT AVAILABLE */
#define EEXFL 013	/* FILE ALREADY EXISTS */
#define EBDCH 014	/* BAD CHANNEL NUMBER */
#define ETMRG 015	/* TOO MANY ARGUMENTS (CALL) */
#define ENAPK 016	/* PACK NOT MOUNTED */
#define ENSDR 020	/* NON-EXISTENT DIRECTORY */
#define ELCDV 021	/* LOCAL DEVICE ONLY */
#define ESCO  022	/* SELF-CONTRADICTORY OPEN */
#define ENAFL 023	/* FILE LOCKED */
#define ETMDR 024	/* M.F.D. FULL */
#define EMCHN 025	/* DEVICE NOT ASSIGNABLE TO THIS PROCESSOR */
#define ERODV 026	/* DEVICE WRITE-LOCKED */
#define ETMLK 027	/* LINK DEPTH EXCEEDED */
#define ETFRG 030	/* TOO FEW ARGUMENTS (CALL) */
#define EROJB 031	/* CAN'T MODIFY JOB */
#define EROPG 032	/* CAN'T GET THAT ACCESS TO PAGE */
#define EBDRG 033	/* MEANINGLESS ARGS */
#define EBDDV 034	/* WRONG TYPE DEVICE */
#define ENSJB 035	/* NO SUCH JOB */
#define EBOJ  036	/* VALID CLEAR OR STORED SET */
#define ENACR 037	/* NO CORE AVAILABLE */
#define ETOP  040	/* NOT TOP LEVEL */
#define ENAPP 041	/* OTHER END OF PIPELINE GONE OR NOT OPEN */
#define ENAJB 042	/* JOB GONE OR GOING AWAY */
#define ENSCL 043	/* ILLEGAL SYSTEM CALL NAME */
#define ENSCH 044	/* CHANNEL NOT OPEN */
#define ENRBF 045	/* INPUT BUFFER EMPTY OR OUTPUT BUFFER FULL */
#define EBDFL 046	/* UNRECOGNIZABLE FILE (LOAD) */
#define EBDLK 047	/* LINK TO NON-EXISTENT FILE */

extern int protoc_init (const char *);
extern int protoc_open (const word_t *, int);
extern int protoc_iclose (void);
extern int protoc_oclose (void);
extern int protoc_read (word_t *, int);
extern int protoc_write (const word_t *, int);
