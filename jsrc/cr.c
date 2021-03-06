/* Copyright 1990-2009, Jsoftware Inc.  All rights reserved.               */
/* Licensed use only. Any other use is in violation of copyright.          */
/*                                                                         */
/* Conjunctions: Rank Associates                                           */

#include "j.h"

#define DR(r)           (0>r?RMAX:r)


I mr(A w){R VAV(w)->mr;}
I lr(A w){R VAV(w)->lr;}
I rr(A w){R VAV(w)->rr;}

// effective rank: ar is rank of argument, r is rank of verb (may be negative)
// result is rank of argument cell
I efr(I ar,I r){R 0>r?MAX(0,r+ar):MIN(r,ar);}

#define NEWYA   {GA(ya,at,acn,lr,as+af); uu=CAV(ya);}
#define NEWYW   {GA(yw,wt,wcn,rr,ws+wf); vv=CAV(yw);}
#define MOVEYA  {MC(uu,u,ak); u+=ak; if(state&STATEAREL)RZ(ya=relocate((I)a-(I)ya,ya));}
#define MOVEYW  {MC(vv,v,wk); v+=wk; if(state&STATEWREL)RZ(yw=relocate((I)w-(I)yw,yw));}

#define EMSK(x) (1<<((x)-1))
#define EXIGENTERROR (EMSK(EVALLOC) | EMSK(EVATTN) | EMSK(EVBREAK) | EMSK(EVINPRUPT) | EMSK(EVFACE) | EMSK(EVWSFULL) | EMSK(EVTIME) | EMSK(EVSTACK) | EMSK(EVSYSTEM) )  // errors that always create failure

#define STATEOUTERREPEATA 0x01
#define STATEINNERREPEATA 0x04
#define STATEINNERREPEATW 0x08
#define STATENORM 0x10
#define STATEFIRST 0x20
#define STATEERR0 0x40
#define STATEERR 0x80
#define STATEARELX 8
#define STATEAREL (1<<STATEARELX)
#define STATEWRELX 9
#define STATEWREL (1<<STATEWRELX)
#define STATENOPOP 0x400   // set if not OK to tpop the stack
#define STATEINCORPORATEDA 0x800
#define STATEINCORPORATEDW 0x1000


#define RCALL   CALL1(f1,yw,fs)
#define RDIRECT (wt&DIRECT)
#define RFLAG   (!(AFLAG(w)&AFNJA+AFSMM+AFREL))
#define RARG    {if(WASINCORP1(y,yw)){cc = 0;NEWYW;} MOVEYW;}
#define RARG1   {if(WASINCORP1(y,yw)){RZ(yw=ca(yw)); vv=CAV(yw);}}
A jtrank1ex(J jt,A w,A fs,I rr,AF f1){PROLOG(0041);A y,y0,yw,z;C*v,*vv;
    I k,mn,n=1,p,*s,wcn,wf,wk,wr,*ws,wt,yn,yr,*ys,yt;I state;
 RZ(w);
 wt=AT(w);
 if(wt&SPARSE)R sprank1(w,fs,rr,f1);
 wr=AR(w); ws=AS(w); rr=efr(wr,rr); wf=wr-rr;
 state=0; if(ARELATIVE(w))state=STATEWREL;
 if(!wf)R CALL1(f1,w,fs);
 PROD(wcn,rr,wf+ws); wk=wcn*bp(wt); v=CAV(w); NEWYW;  // if this PROD overflows, the reshape below will fail
 p=wf; s=ws; mn=prod(wf,ws);
 if(AN(w))MOVEYW else RZ(yw=reshape(vec(INT,rr,ws+wf),filler(w)));

// Assignments from cr.c:
// ?r=rank, ?s->shape, ?cr=effective rank, ?f=#frame, ?b=relative flag, for each argument
// ?cn=number of atoms in a cell, ?k=#bytes in a cell, uv point to one cell before aw data
// Allocate y? to hold one cell of ?, with uu,vv pointing to the data of y?
// b means 'w frame is larger'; p=#larger frame; q=#shorter frame; s->larger frame
// mn=#cells in larger frame (& therefore #cells in result); n=# times to repeat each cell
//  from shorter-frame argument

{B cc=1;C*zv;I j=0,jj=0,old;
 if(mn){y0=y=RCALL; RZ(y);}  // if there are cells, execute on the first one
 else{I d;
  // if there are no cells, execute on a cell of fills.  Do this quietly, because
  // if there is an error, we just want to use a value of 0 for the result; thus debug
  // mode off and RESETERR on failure.
  // However, if the error is a non-computational error, like out of memory, it
  // would be wrong to ignore it, because the verb might execute erroneously with no
  // indication that anything unusual happened.  So fail then
  d=jt->db; jt->db=0; y=RCALL; jt->db=d;
  if(jt->jerr){if(EMSK(jt->jerr)&EXIGENTERROR)RZ(y); y=zero; RESETERR;}
 } 

 // yt=type, yr=rank, ys->shape, yn=#atoms k=#bytes of first-cell result
 yt=AT(y); yr=AR(y); ys=AS(y); yn=AN(y); k=yn*bp(yt);
 // First shot: zip through the cells, laying the results into the output area
 // one by one.  We can do this if the results are direct (i. e. not pointers),
 // or if there are no results at all; and we can continue until we hit an incompatible result-type.
 // With luck this will process the entire input.
 if(!mn||yt&DIRECT&&RFLAG){I zn;
  RARG1; RE(zn=mult(mn,yn));   // Reallocate y? if needed; zn=number of atoms in all result cells (if they stay homogeneous)
  GA(z,yt,zn,p+yr,0L); ICPY(AS(z),s,p); ICPY(p+AS(z),ys,yr);  // allocate output area, move in long frame followed by result-shape
  if(mn){zv=CAV(z); MC(zv,AV(y),k);}   // If there was a first cell, copy it in
  // Establish the point we will free to after each call.  This must be after the allocated result area, and
  // after the starting result cell.  After we call the verb we will free up what it allocated, until we have to
  // reallocate the result cell; then we would be wiping out a cell that we ourselves allocated, so we stop
  // freeing then
  old=jt->tnextpushx;
  for(j=1;j<mn;++j){   // for each additional result-cell...
   RARG;    // establish argument cells
   RZ(y=RCALL);  // call the function
   if(TYPESNE(yt,AT(y))||yr!=AR(y)||yr&&ICMP(AS(y),ys,yr))break;  // break if there is a change of cell type/rank/shape
   MC(zv+=k,AV(y),k);   // move the result-cell to the output
   if(cc)tpop(old);  // Now that we have copied to the output area: if we have not reallocated a cell on the stack, free what the verb did
  }
 }
 if(j<mn){A q,*x,yz;
  // Here we were not able to build the result in the output area; type/rank/shape changed.
  // We will create a boxed result, boxing each cell, and then open it.  If this works, great.
  jj=j%n;   // j = #cells we processed before the wreck; jj=position in the smaller cell (not used for monad; compiler should optimize it away)
  GATV(yz,BOX,mn,p,s); x=AAV(yz);   // allocate place for boxed result
  // For each previous result, put it into a box and store the address in the result area
  if(j){
   zv=CAV(z)-k;
   DO(j, GA(q,AT(y0),AN(y0),AR(y0),AS(y0)); MC(AV(q),zv+=k,k); *x++=q;);  // We know the type/shape/rank of y0 matches them all
  }
  *x++=y;   // move in the result that caused the wreck
  DO(mn-j-1, RARG; RZ(y=RCALL); *x++=y;);   // for all the rest, execute the cells and move pointer to output area
  z=ope(yz);  // We have created x <@f y; this creates > x <@f y which is the final result
 }
 EPILOG(z);  // If the result is boxed, we know we had no wastage at this level except for yz, which is small compared to z
}
}

// obsolete #define RCALL   CALL2(f2,ya,yw,fs)
// obsolete #define RDIRECT (at&DIRECT&&wt&DIRECT)
// obsolete #define RFLAG   (!(AFLAG(a)&AFNJA+AFSMM+AFREL)&&!(AFLAG(w)&AFNJA+AFSMM+AFREL))
// obsolete #define RARG    {if(++jj==n)jj=0; \
// obsolete                  if(!b||jj==0){if(y==ya||ACUC1<AC(ya)){cc = 0;NEWYA;} MOVEYA;}  \
// obsolete                  if( b||jj==0){if(y==yw||ACUC1<AC(yw)){cc = 0;NEWYW;} MOVEYW;} }
// obsolete // If the use-count in y? has been incremented, it means that y? was incorporated into an indirect
// obsolete // noun and must not be modified.  In that case, we reallocate it.  This is used to reallocate the
// obsolete // first cell only.
// obsolete #define RARG1   {if(y==ya||ACUC1<AC(ya)){RZ(ya=ca(ya)); uu=CAV(ya);}  \
// obsolete                  if(y==yw||ACUC1<AC(yw)){RZ(yw=ca(yw)); vv=CAV(yw);}}


// General setup for verbs that do not go through jtirs[12].  Some of these are marked as IRS verbs.  General
// verbs derived from u"n also come through here, via jtrank2.
// A verb u["n] using this function checks to see whether it has multiple cells; if so,
// it calls here, giving a callback; we split the arguments into cells and call the callback,
// which is often the same original function that called here.
A jtrank2ex(J jt,A a,A w,A fs,I lr,I rr,I lcr,I rcr,AF f2){PROLOG(0042);A y,ya,yw,z;
   C*u,*uu,*v,*vv;I acn,af,ak,ar,*as,at,k,mn,n=1,wcn,wf,wk,wr,*ws,wt,yn,yr,*ys,yt;
 I outerframect, outerrptct, innerframect, innerrptct, aof, wof, sof, lof, sif, lif, *lis, *los;

 RZ(a&&w);
 at=AT(a); wt=AT(w);
 if(at&SPARSE||wt&SPARSE)R sprank2(a,w,fs,lcr,rcr,f2);  // this needs to be updated to handle multiple ranks
// lr,rr are the ranks of the underlying verb.  lcr,rcr are the cell-ranks given by u"lcr rcr.
// If " was not used, lcr,rcr=lr,rr
// When processing v"l r the shapes look like:
// a frame   x x O  | x x x
//                   <---l-->
// w frame   x x    | x x x I
//                   <---r-->
// the outer frame is to the left of the |, inner frame to the right.
// the rank of v is not included; the frames shown above pick up after that.  There are two
// possible repetitions required: if there is mismatched frame BELOW the rank (l r), as shown by letter I above,
// the individual cells of the shorter-frame argument must be repeated.  innerrptct gives the
// number of times the cell should be repeated.  If there is mismatched frame ABOVE the rank (l r), as
// shown by letter O above, the rank-(l/r) cell of the short-frame argument must be repeated.  outerrptct
// tells how many times the cell should be repeated; outerrestartpt gives the address of the rank-(l/r) cell
// being repeated; outercellct gives the number of (below lr) cells that are processed before an outer repetition.
// The two repeats can be for either argument independently, depending on which frame is shorter.

 // ?r=rank, ?s->shape, ?cr=effective rank, ?f=#total frame (inner+outer), ?b=relative flag, for each argument
 // if inner rank is > outer rank, set it equal to outer rank
 I state=STATEFIRST;  // initial state: working on first item, OK to pop stack, etc,
 ar=AR(a); as=AS(a); lr=efr(ar,lr); lcr=efr(ar,lcr); if(lr>lcr)lr=lcr; af=ar-lr; if(ARELATIVE(a))state|=STATEAREL;
 wr=AR(w); ws=AS(w); rr=efr(wr,rr); rcr=efr(wr,rcr); if(rr>rcr)rr=rcr; wf=wr-rr; if(ARELATIVE(w))state|=STATEWREL;
 if(!af&&!wf){R CALL2(f2,a,w,fs);}  // if there's only one cell and no frame, run on it, that's the result.  Should not occur
 // multiple cells.  Loop through them.

 // Get the length of the outer frames, which are needed only if either "-rank is greater than the verb rank,
 // either argument has frame with respect to the "-ranks, and those frames are not the same length
 aof=ar-lcr; wof=wr-rcr;   // ?of = outer frame
 if(!((lcr>lr||rcr>rr)&&((aof>0)||(wof>0))&&aof!=wof)){los=0; lof=aof=wof=0; outerframect=outerrptct=1;  // no outer frame unless it's needed
 }else{
  // outerframect is the number of cells in the shorter frame; outerrptct is the number of cells in the residual frame
  if(aof>wof){wof&=~(wof>>(BW-1)); lof=aof; sof=wof; los=as;}else{aof&=~(aof>>(BW-1)); lof=wof; sof=aof; los=ws; state|=STATEOUTERREPEATA;}  // clamp smaller frame at min=0
  ASSERT(!ICMP(as,ws,sof),EVLENGTH);  // prefixes must agree
  RE(outerframect=prod(sof,los)); RE(outerrptct=prod(lof-sof,los+sof));  // get # cells in frame, and in unmatched frame
 }

 // Now work on inner frames.  Compare frame lengths after discarding outer frames
 // set lif=length of longer inner frame, sif=length of shorter inner frame, lis->longer inner shape
 if((af-aof)>(wf-wof)){
  // a has the longer inner frame.  Repeat cells of w
  lif=af-aof; sif=wf-wof; lis=as+aof;
  state |= STATEINNERREPEATW;
 } else if((af-aof)<(wf-wof)){
  // w has the longer inner frame.  Repeat cells of a
  lif=wf-wof; sif=af-aof; lis=ws+wof;
  state |= STATEINNERREPEATA;
 } else{
  // inner frames are equal.  No repeats
  lif=wf-wof; sif=af-aof; lis=ws+wof;
 }
 ASSERT(!ICMP(as+aof,ws+wof,sif),EVLENGTH);  // error if frames are not same as prefix
 RE(innerrptct=prod(lif-sif,lis+sif));  // number of repetitions per matched-frame cell
 RE(innerframect=prod(sif,lis));   // number of cells in matched frame

 // Migrate loops with count=1 toward the inner to reduce overhead.  We choose not to promote the outer to the inner if both
 // innerframect & innerrptct are 1, on grounds of rarity
 if(innerrptct==1){innerrptct=innerframect; innerframect=1; state &=~(STATEINNERREPEATW|STATEINNERREPEATA);}  // if only one loop needed, make it the inner, with no repeats


// obsolete  // b means 'w frame is larger'; p=#larger frame; q=#shorter frame; s->larger frame
// obsolete  // mn=#cells in larger frame (& therefore #cells in result); n=# times to repeat each cell
// obsolete  //  from shorter-frame argument
// obsolete  b=af<=wf; p=b?wf:af; q=b?af:wf; s=b?ws:as; RE(mn=prod(p,s)); RE(n=prod(p-q,s+q));
// obsolete  ASSERT(!ICMP(as,ws,q),EVLENGTH);  // error if frames are not same as prefix
 // Set up y? with the next cell data.  The data might be unchanged from the previous, for the argument
 // with the shorter frame.  Whenever we have to copy, we first
 // check to see if the cell-workarea has been incorporated into a result noun; if so, we have to
 // reallocate.  We assume that the cell-workarea is not modified by RCALL, because we reuse it in situ
 // when a cell is to be repeated.  NEWY? allocates a new argument cell, and MOVEY? copies to it.
 // b&1 is set if the inner repeat is for w, b&2 is set if the outer repeat is for w.
 // innerphase tells where we are in in inner repetition cycle.  When it hits 0, we advance, otherwise repeat.
 // outerphase[01] tell where we are in the outer repetition cycle.  When outerphase0 hits 0, we do one check of
 //  outerphase1, advancing if it hits 0, repeating otherwise
 // The phase counters are advanced by RARG and not touched by RCALL, so that if we have to switch loops we can continue using
 // them.

 // Get size of each argument cell in atoms.  If this overflows, there must be a 0 in the frame, & we will have
 // gone through the fill path (& caught the overflow)
 PROD(acn,lr,as+af); PROD(wcn,rr,ws+wf);
 // Allocate workarea y? to hold one cell of ?, with uu,vv pointing to the data area y?
 // ?cn=number of atoms in a cell, ?k=#bytes in a cell, uv->aw data
 ak=acn*bp(at); u=CAV(a); NEWYA;  // reshape below will catch any overflow
 wk=wcn*bp(wt); v=CAV(w); NEWYW;

 // See how many atoms are going to be in the result
 RE(mn=mult(mult(outerframect,outerrptct),mult(innerframect,innerrptct)));

 if(!mn){I d, *is, *zs;
  // if there are no cells, execute on a cell of fills.
  if(AN(a))MOVEYA else RZ(ya=reshape(vec(INT,lr,as+af),filler(a)));
  if(AN(w))MOVEYW else RZ(yw=reshape(vec(INT,rr,ws+wf),filler(w)));
  // Do this quietly, because
  // if there is an error, we just want to use a value of 0 for the result; thus debug
  // mode off and RESETERR on failure.
  // However, if the error is a non-computational error, like out of memory, it
  // would be wrong to ignore it, because the verb might execute erroneously with no
  // indication that anything unusual happened.  So fail then
  d=jt->db; jt->db=0; y=CALL2(f2,ya,yw,fs); jt->db=d;
  if(jt->jerr){if(EMSK(jt->jerr)&EXIGENTERROR)RZ(y); y=zero; RESETERR;}  // use 0 as result if error encountered
  GA(z,AT(y),0L,lof+lif+AR(y),0L); zs=AS(z);
  is = los; DO(lof, *zs++=*is++;);  // copy outer frame
  is = lis; DO(lif, *zs++=*is++;);  // copy inner frame
  is = AS(y); DO(AR(y), *zs++=*is++;);    // copy result shape
 }else{I i0, i1, i2, i3, old;C *zv;
  // Normal case where there are cells.
  // loop over the matched part of the outer frame
  for(i0=outerframect;i0;--i0){
   C *outerrptstart=state&STATEOUTERREPEATA?u:v;
   // loop over the unmatched part of the outer frame, repeating the shorter argument
   for(i1=outerrptct;i1;--i1){  // make MOVEY? post-increment
    if(!(state&STATEOUTERREPEATA))v=outerrptstart;
    else u=outerrptstart;  // if we loop, we know we must be repeating one or the other
    // loop over the matched part of the inner frame
    for(i2=innerframect;i2;--i2){
     // establish argument cells by using MOVEY? to move the cell into the workarea y?.  Before we do that,
     // we have to see whether the previous value in y? was incorporated into a result (which it is if it IS
     // the result, or if its usecount has been incremented).  If it has been so incorporated, we must reallocate
     // the workarea and also stop freeing blocks allocated up the stack, lest we free our workarea as well
     // if an argument is going to be repeated, establish it here
     if((state&STATEINNERREPEATA)){if(state&STATEINCORPORATEDA){state&=~(STATEINCORPORATEDA);NEWYA;} MOVEYA;}
     if((state&STATEINNERREPEATW)){if(state&STATEINCORPORATEDW){state&=~(STATEINCORPORATEDW);NEWYW;} MOVEYW;}
     // loop over the unmatched part of the inner frame, repeating the shorter argument
     for(i3=innerrptct;i3;--i3){
      // establish any argument that is not going to be repeated
      if(!(state&STATEINNERREPEATA)){if(state&STATEINCORPORATEDA){state&=~(STATEINCORPORATEDA);NEWYA;} MOVEYA;}
      if(!(state&STATEINNERREPEATW)){if(state&STATEINCORPORATEDW){state&=~(STATEINCORPORATEDW);NEWYW;} MOVEYW;}
      // invoke the function, get the result for one cell
      RZ(y=CALL2(f2,ya,yw,fs));
      // see if the workarea was incorporated into the result, for use next time through the loop
      if(WASINCORP1(y,ya))state|=STATEINCORPORATEDA|STATENOPOP;
      if(WASINCORP1(y,yw))state|=STATEINCORPORATEDW|STATENOPOP;

      // process according to state
      if(state&STATENORM){
       // Normal case: not first time, no error found yet.  Move verb result to its resting place.  zv points to the next output location
       if(TYPESNE(yt,AT(y))||yr!=AR(y)||yr&&ICMP(AS(y),ys,yr)){state^=(STATENORM|STATEERR0);}  //switch to ERR0 state if there is a change of cell type/rank/shape
       else{
        // Normal path.  
        MC(zv,AV(y),k); zv+=k;  // move the result-cell to the output, advance to next output spot
        if(!(state&STATENOPOP))tpop(old);  // Now that we have copied to the output area, free what the verb allocated
       }
      }

      if(state&STATEFIRST){I *is, zn;
       // Processing the first cell.  Allocate the result area now that we know the shape/type of the result.  If the result is not DIRECT,
       // or if an argument is memory-mapped, we have to go through the box/unbox drill to make sure that the input argument is not incorporated
       // as is into a result.  In that case, we switch this allocation to be a single box per result-cell, to avoid having to reallocate
       // immediately
       yt=AT(y);  // type of the first result
       if(yt&DIRECT&&!((AFLAG(a)|AFLAG(w))&(AFNJA|AFSMM|AFREL))){
         yr=AR(y); yn=AN(y);
         RE(zn=mult(mn,yn));   // zn=number of atoms in all result cells (if they stay homogeneous)
       }else{
         yt=BOX; yr=0; zn=mn;
       }
       GA(z,yt,zn,lof+lif+yr,0L); I *zs=AS(z); zv=CAV(z);
       is = los; DO(lof, *zs++=*is++;);  // copy outer frame
       is = lis; DO(lif, *zs++=*is++;);  // copy inner frame
       if(!(yt&BOX)){   // if we are going to be able to run normal case...
        ys=AS(y); k=yn*bp(yt);   // save info about the first cell for later use
        is = AS(y); DO(yr, *zs++=*is++;);    // copy result shape
        MC(zv,AV(y),k); zv+=k;   // If there was a first cell, copy it in & advance to next output spot
        old=jt->tnextpushx;  // pop back to AFTER where we allocated our result and argument blocks
        state^=(STATEFIRST|STATENORM);  // advance to STATENORM
       }else {state^=(STATEFIRST|STATEERR);}  // advance to STATEERR since we have switched gears already
      }

      if(state&(STATEERR0|STATEERR)){
       if(state&STATEERR0){
        // We had a wreck.  Either the first cell was not direct, or there was a change of type.  We cope by boxing
        // each individual result, so that we can open them at the end to produce a single result (which might fail when opened)
        // It would be nice if boxed results didn't go through this path
        // If the result is boxed, it means we detected the wreck before the initial allocation.  The initial allocation
        // is the boxed area where we build <"0 result, and zv points to the first box pointer.  We have nothing to adjust.
        C *zv1=CAV(z);   // pointer to cell data
        GATV(z,BOX,mn,lof+lif,AS(z)); A *x=AAV(z);   // allocate place for boxed result; copy frame part of result-shape
        // For each previous result, put it into a box and store the address in the result area
        // We have to calculate the number of cells, rather than using the output address, because the length of a cell may be 0
        // wrecki does not include the cell that caused the wreck
        I wrecki = (innerrptct-i3) + innerrptct * ( (innerframect-i2) + innerframect * ( (outerrptct-i1) + outerrptct * (outerframect-i0) ) );
        DQ(wrecki , A q; GA(q,yt,yn,yr,ys); MC(AV(q),zv1,k); zv1+=k; *x++=q;)  // We know the type/shape/rank of the first result matches them all
        // from now on the main output pointer, zv, points into the result area for z
        zv = (C*)x;
        state^=(STATEERR0|STATEERR);  // advance to STATEERR
       }
       // Here for all errors, including the first after it has cleaned up the mess, and for boxed result the very first time with no mess
       *(A*)zv=y; zv+=sizeof(A*);   // move in the most recent result, advance pointer to next one
      }
     }
    }
   }
  }
 }
 if(state&STATEERR)z=ope(z);  // If we went to error state, we have created x <@f y; this creates > x <@f y which is the final result
 EPILOG(z);
}

/* Integrated Rank Support                              */
/* f knows how to compute f"r                           */
/* jt->rank points to a 2-element vector of             */
/* (left, right (monadic)) ranks                        */
/* 0=jt->rank means f is not being called from rank     */
/* jt->rank is guaranteed positive                      */
/* jt->rank is guaranteed <: argument ranks             */
/* frame agreement is verified before invoking f        */
/* frames either match, or one is empty                 */
/* (i.e. prefix agreement invokes general case)         */
// If the action verb handles inplacing, we pass that through

A jtirs1(J jt,A w,A fs,I m,AF f1){A z;I*old,rv[2],wr; 
 F1PREFIP; RZ(w);
 wr=AR(w); rv[1]=m=efr(wr,m);
 if(fs&&!(VAV(fs)->flag&VINPLACEOK1))jtinplace=jt;  // pass inplaceability only if routine supports it
 if(m>=wr)R CALL1IP(f1,w,fs);
 rv[0]=0;
 old=jt->rank; jt->rank=rv; z=CALL1IP(f1,w,fs); jt->rank=old; 
 R z;
}

// IRS setup for dyads x op y
// a is x, w is y
// fs is the f field of the verb (the verb to be applied repeatedly) - or 0 if none
//  if inplacing is enabled in jt, fs must be given
// l, r are nominal ranks of fs
// f2 is a setup verb (jtover, jtreshape, etc)
// IRS verbs are those that look at jt->rank.  This is where we set up jt->rank.  Once
// we have it, we call the setup verb, which will go on to do its internal looping and (optionally) call
// the verb f2 to finish operation on a cell
A jtirs2(J jt,A a,A w,A fs,I l,I r,AF f2){A z;I af,ar,*old,rv[2],wf,wr;
 // push the jt->rank (pointer to ranks) stack.  push/pop may not match, no problem
 F1PREFIP; RZ(a&&w);
 ar=AR(a); rv[0]=l=efr(ar,l); af=ar-l;  // get rank, effective rank of u"n, length of frame...
 wr=AR(w); rv[1]=r=efr(wr,r); wf=wr-r;     // ...for both args
 if(fs&&!(VAV(fs)->flag&VINPLACEOK2))jtinplace=jt;  // pass inplaceability only if routine supports it
 if(!(af||wf))R CALL2IP(f2,a,w,fs);   // if no frame, call setup verb and return result
 ASSERT(!ICMP(AS(a),AS(w),MIN(af,wf)),EVLENGTH);   // verify agreement
 old=jt->rank; jt->rank=rv; z=CALL2IP(f2,a,w,fs); jt->rank=old;   // save ranks, call setup verb, pop rank stack
  // Not all setup verbs (*f2)() use the fs argument.  
 R z;
}


static DF1(cons1a){R VAV(self)->f;}
static DF2(cons2a){R VAV(self)->f;}

// Constant verbs do not inplace because we loop over cells.  We could speed this up if it were worthwhile.
static DF1(cons1){V*sv=VAV(self);
 RZ(w);
 R rank1ex(w,self,efr(AR(w),*AV(sv->h)),cons1a);
}
static DF2(cons2){V*sv=VAV(self);I*v=AV(sv->h);
 RZ(a&&w);
 R rank2ex(a,w,self,AR(a),AR(w),efr(AR(a),v[1]),efr(AR(w),v[2]),cons2a);
}

// Handle u"n y where u supports irs.  Since the verb may support inplacing even with rank (,"n for example), pass that through.
// If inplacing is allowed here, pass that on to irs.  It will see whether the action verb can support inplacing.
// NOTHING HERE MAY DEREFERENCE jt!!
static DF1(rank1i){DECLF;A h=sv->h;I*v=AV(h); R irs1(w,fs,*v,f1);}
static DF2(rank2i){DECLF;A h=sv->h;I*v=AV(h); R irs2(a,w,fs,v[1],v[2],f2);}

// u"n y when u does not support irs. We loop over cells, and as we do there is no reason to enable inplacing
static DF1(rank1){DECLF;A h=sv->h;I m,*v=AV(h),wr;
 RZ(w);
 wr=AR(w); m=efr(wr,v[0]);
 R m<wr?rank1ex(w,fs,m,f1):CALL1(f1,w,fs);
}

// For the dyads, rank2ex does a quadruply-nested loop over two rank-pairs, which are the n in u"n (stored in h) and the rank of u itself (fetched from u).
static DF2(rank2){DECLF;A h=sv->h;I ar,l=AV(h)[1],r=AV(h)[2],wr;
 RZ(a&&w);
 ar=AR(a); l=efr(ar,l);
 wr=AR(w); r=efr(wr,r);
 if(l<ar||r<wr) {I llr=VAV(fs)->lr, lrr=VAV(fs)->rr;  // fetch ranks of werb we are going to call
  // if the verb we are calling is another u"n, we can skip coming through here a second time & just go to the f2 for the nested rank
  if(f2==rank2&&!(AT(a)&SPARSE||AT(w)&SPARSE)){fs = VAV(fs)->f; f2=VAV(fs)->f2;}
  R rank2ex(a,w,fs,llr,lrr,l,r,f2);
 }else R CALL2(f2,a,w,fs);  // pass in verb ranks to save a level of rank processing if not infinite
}


// a"w; result is a verb
F2(jtqq){A h,t;AF f1,f2;D*d;I *hv,n,r[3],vf,*v;
 RZ(a&&w);
 // The h value in the function will hold the ranks from w.  Allocate it
 GAT(h,INT,3,1,0); hv=AV(h);  // hv->rank[0]
 if(VERB&AT(w)){
  // verb v.  Extract the ranks into a floating-point list
  GAT(t,FL,3,1,0); d=DAV(t);
  n=r[0]=hv[0]=mr(w); d[0]=n<=-RMAX?-inf:RMAX<=n?inf:n;
  n=r[1]=hv[1]=lr(w); d[1]=n<=-RMAX?-inf:RMAX<=n?inf:n;
  n=r[2]=hv[2]=rr(w); d[2]=n<=-RMAX?-inf:RMAX<=n?inf:n;
  // The floating-list is what we will call the v operand
  // h is the integer version
  w=t;
 }else{
  // Noun v. Extract and turn into 3 values, stored in h
  n=AN(w);
  ASSERT(1>=AR(w),EVRANK);
  ASSERT(0<n&&n<4,EVLENGTH);
  RZ(t=vib(w)); v=AV(t);
  hv[0]=v[2==n]; r[0]=DR(hv[0]);
  hv[1]=v[3==n]; r[1]=DR(hv[1]);
  hv[2]=v[n-1];  r[2]=DR(hv[2]);
 }

 // Get the action routines and flags to use for the derived verb
 if(NOUN&AT(a)){f1=cons1; f2=cons2; ACIPNO(a);// use the constant routines for nouns; mark the constant non-inplaceable since it may be reused;
  // Temporarily raise the usecount of the noun.  Because we are in the same tstack frame as the parser, the usecount will stay
  // raised until any inplace decision has been made regarding this derived verb, protecting the derived verb if the
  // assigned name is the same as a name appearing here.  If the derived verb is used in another sentence, it must first be
  // assigned to a name, which will protects values inside it.
  rat1s(a);
  vf=VASGSAFE;    // the noun can't mess up assignment, and does not support IRS
 }else{
  // The flags for u indicate its IRS and atomic status.  If atomic (for monads only), ignore the rank, just point to
  // the action routine for the verb.  Otherwise, choose the appropriate rank routine, depending on whether the verb
  // supports IRS.  The IRS verbs may profitably support inplacing, so we enable it for them.
  V* av=VAV(a);   // point to verb info
  vf=av->flag&VASGSAFE;  // inherit ASGSAFE from u
  if(av->flag&VISATOMIC1){f1=av->f1;}else if(av->flag&VIRS1){f1=rank1i;vf|=VINPLACEOK1;}else{f1=rank1;}
  if(av->flag&VIRS2){f2=rank2i;vf|=VINPLACEOK2;}else{f2=rank2;}
 }

 // Create the derived verb.  The derived verb (u"n) NEVER supports IRS; it inplaces if the action verb u supports irs
 R fdef(CQQ,VERB, f1,f2, a,w,h, vf, r[0],r[1],r[2]);
}
