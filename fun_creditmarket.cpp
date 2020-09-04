//#define EIGENLIB			// uncomment to use Eigen linear algebra library
//#define NO_POINTER_INIT	// uncomment to disable pointer checking

#include "fun_head_fast.h"

// do not add Equations in this area

MODELBEGIN

/////////////banks and firms variables//////////////////////

EQUATION("_NWb")
//individual bank net worth

v[0] = VL("_NWb", 1);
v[1] = uniform(0.9, 1.1);

RESULT(v[0] * v[1])

EQUATION("_loanb")
//individual bank loans

V("cred_mkt");
v[0] = VL("_loanb", 1);
v[1] = V("new_loanb");

RESULT(v[0] + v[1])

EQUATION("_levb")
 //banks leverage

v[0] = VL("_loanb", 1);
v[1] = VL("_NWb", 1);

RESULT(v[0]/v[1] )

EQUATION("_NWf")
//individual firm net worth

v[0] = VL("_NWf", 1);
v[1] = uniform(0.9, 1.1);

RESULT(v[0] * v[1])

EQUATION("_loanf")
//individual firm loans

V("cred_mkt");
v[0] = VL("_loanf", 1);
v[1] = V("new_loanf");

RESULT(v[0] + v[1])

EQUATION("_levf")
 //firms leverage

v[0] = VL("_loanf", 1);
v[1] = VL("_NWf", 1);

RESULT(v[0]/v[1] )

EQUATION("_tlev")
//individual firm target leverage

v[0] = VL("_tlev", 1);
v[1] = uniform(0.9, 1.1);

RESULT(v[0] * v[1])

//////////////////////////////////////////////////////////////////////////////////////////////

EQUATION("setBanks")
//choose the initial set of banks the firm will apply for credit

v[0] = max( round( V("fsig") * V("B") ), 1 ); //# of banks in each firm's set of target banks

CYCLE(cur, "firm")
{
	for ( j = 1; j <= v[0] ; ++j )
	{
		do
		{
			cur5=RNDDRAW("bank","_NWb");
			v[3] = VS( cur5, "_IDb" );
			cur3=SEARCH_CNDS(cur, "IDb", v[3]);
		}
		while ( cur3 != NULL );
	
		if ( j == 1 )		// pick first existing object
			cur1 = SEARCHS( cur, "CliB" );
		else
			cur1 = ADDOBJLS( cur, "CliB", T - 1 );
			
		WRITE_SHOOKS( cur1, cur5 );
		WRITES(cur1, "IDb", v[3]);
	}
}

PARAMETER;

RESULT(1 )

EQUATION("up_setBanks")
//update the set of target banks

V("setBanks");
V("new_loanf");
v[0] = COUNT("CliB");
v[6] = round( V("fsig") * V("B") );
if ( v[0] < v[6] )
{
	//for ( j = 1; j < (v[6]-v[0]); ++j )
	do
	{
		do
		{
			cur5=RNDDRAW("bank", "_NWb"); //choose random bank
			v[7] = VS( cur5, "_IDb" ); //ID of chosen bank
			cur6=SEARCH_CND("IDb", v[7]);
		}
		while ( cur6 != NULL );
		
		cur7 = ADDOBJ( "CliB");
		WRITES( cur7, "IDb", v[7] );
		WRITE_SHOOKS( cur7, cur5 );
		v[0]+=1;
	}
	while (v[0] <= v[6] );
}

for ( j = 1; j <= 1 ; ++j )
{
	v[1] = MAX("levb"); //max. leverage
	do
	{
		cur2=RNDDRAW("bank", "_NWb"); //choose random bank
		v[3] = VS( cur2, "_IDb" ); //ID of chosen bank
		cur3=SEARCH_CND("IDb", v[3]);
	}
	while( cur3 != NULL );

	v[4] = VS( cur2, "_levb" ); //leverage of chosen bank

	if ( v[4]<v[1] )
	{
		cur4 = ADDOBJL( "CliB", T - 1 );
		WRITES( cur4, "IDb", v[3] );
		WRITE_SHOOKS( cur4, cur2 );
	
		cur1 = SEARCH_CND("levb", v[1]);
	
		LOG( "\nfirm=%g replacing bank=%g (levb=%g) by bank=%g (levb=%g)", V( "_IDf" ), VS( cur1, "IDb" ), VS( cur1, "levb" ), VS( cur4, "IDb" ), VS( cur4, "levb" ) );

		DELETE(cur1);
	}
}

RESULT(v[7] )

EQUATION("max_loanf")
/*
Desired loan by firm in period
*/

v[0] = V( "_tlev");
v[1] = V( "_NWf");
v[2] = VL( "_loanf", 1);

if ( v[0]*v[1] > v[2] )
	v[3] = (v[0]*v[1]) - v[2];
else
	v[3] = 0;

RESULT( v[3] )

EQUATION("max_loanb")
/*
Maximum loan by bank in period
*/

v[0] = V( "kreq"); //maximum bank leverage
v[1] = V( "_NWb");
v[2] = VL( "_loanb", 1);

if ( v[0]*v[1] > v[2] )
	v[3] = (v[0]*v[1]) - v[2];
else
	v[3] = 0;

RESULT( v[3] )


EQUATION("levb")
/*
Bank leverage copy at client firm
*/

LOG( "\n updated bank=%g | _levb=%g ", VS( SHOOK, "_IDb" ), VS( SHOOK, "_levb" ) );

RESULT( VS( SHOOK, "_levb" ) )


EQUATION("cred_mkt")
//initialize the credit market

SORT("firm","_levf","UP");//sort firms according to their leverage in ascending order
CYCLE( cur, "bank" )
	VS( cur, "new_loanb" );

RESULT(1 )


EQUATION("new_loanf")
/*
New loan taken by firm in period
*/

V( "cred_mkt" );
//V("max_loanb");
LOG( "\nfirm=%g updating bank list", V( "_IDf" ) );

v[0] = 0;
v[2] = V("max_loanf");
SORT("CliB", "levb", "UP");//set firms' target banks according to their leverage in ascending order
	
LOG( "\nfirm=%g picking loans", V( "_IDf" ) );
	
CYCLE( cur1, "CliB")
{
		cur2 = SHOOKS( cur1 );
		v[1] = VS(cur2, "_NWb");
		v[4] = VS(cur2, "max_loanb");
		v[5] = min( 0.25 * v[1], v[4] ); //loan can't be greater than 25% of bank's NW
		if ( v[5] < 0.05 * v[2] )
		{
			v[6] = 0;
			DELETE(cur1);
		}
		else
			v[6] = v[5]; //bank will grant loan to the firm if it can provide at least 5% of firm's credit demand
		if ( v[6] < v[2] )
			v[0] += v[3] = v[6];
		else
			v[0] += v[3] = v[2];

		//LOG( "\n picked bank=%g | levb=%g | _NWb=%g | delta_new_loanb=%g | max_loanb=%g", VS( cur1, "IDb" ), VS( cur1, "levb" ), v[1], v[3], v[4] );

		INCRS(cur2, "new_loanb", v[3]);
		INCRS(cur2, "max_loanb", -v[3]);
		if ( v[3]>0 )
		{
			cur3 = ADDOBJ("cred");
			WRITES(cur3, "value", v[3]);
			WRITES(cur3, "period", 0);
			WRITE_SHOOKS(cur3, cur2);
		}
		v[2] -= v[3];
		if ( v[2] <= 0 )
			break;
}

RESULT( v[0] )

EQUATION("period")
//update credit period
v[0] = VL("period", 1);

RESULT(v[0]+1 )

EQUATION("value")
//update credit value

V("period");
v[0] = VL("value", 1);
v[1] = V("nper");
v[2] = V("period");
if ( v[2] == 0 )
	v[5] = 1;
else
	v[3] = v[1]+1-v[2];
	v[4] = 1/v[3];
	v[5] = 1 - v[4];

RESULT( v[0] * v[5] )

EQUATION("up_cred")
//delete credit contracts (obj. "cred") with value equal to zero

V("value");
CYCLE(cur, "cred")
{
	v[0] = VS(cur, "value");
	if ( v[0] == 0)
	{
		DELETE(cur);
	}
}

//cur = SEARCH_CND("value", 0);
//if (cur != NULL)
//{
//	DELETE(cur);
//}

RESULT(1 )

EQUATION("totLoanF")
//Total firms loan
RESULT(SUM("new_loanf"))

EQUATION("new_loanb")
RESULT(0 )

EQUATION("totLoanB")
//Total banks loan

CYCLE( cur, "firm" )
	VS( cur, "new_loanf" );
	
RESULT(SUM("new_loanb"))

MODELEND

// do not add Equations in this area

void close_sim( void )
{
	// close simulation special commands go here
}
