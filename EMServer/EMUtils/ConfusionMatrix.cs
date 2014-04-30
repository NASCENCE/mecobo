using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace CGPIP2
{
   public class ConfusionMatrix
   {
      public ulong TP = 0;
      public ulong FP = 0;
      public ulong TN = 0;
      public ulong FN = 0;

      public double Accuracy
      {
         get
         {
            return (double)(TP + TN) / (TP + TN + FN + FP);
         }
      }

      public static ulong Isqrt(ulong num)
      {
         if (0 == num) { return 0; }  // Avoid zero divide
         ulong n = (num / 2) + 1;       // Initial estimate, never low
         ulong n1 = (n + (num / n)) / 2;
         while (n1 < n)
         {
            n = n1;
            n1 = (n + (num / n)) / 2;
         } // end while
         return n;
      } // end Isqrt()

      public static decimal Isqrt(decimal num)
      {
         if (0 == num) { return 0; }  // Avoid zero divide
         decimal n = (num / 2) + 1;       // Initial estimate, never low
         decimal n1 = (n + (num / n)) / 2;
         while (n1 < n)
         {
            n = n1;
            n1 = (n + (num / n)) / 2;
         } // end while
         return n;
      } // end Isqrt()

      public double MCC
      {
         get
         {
           // return MCCFast;

             
             if (Accuracy > 1d - SmallNumber)
             {
                 return 1;
             }
             if (Accuracy < SmallNumber)
             {
                 return -1;
             }
          
             if ((TP + FP == 0) || (TP + FN == 0) || (TN + FP == 0) || (TN + FN == 0))
                 return 0;

             BigNumber.StdBigNumber xbigTP = new BigNumber.StdBigNumber((long)(TP ));
             BigNumber.StdBigNumber xbigFP = new BigNumber.StdBigNumber((long)(FP ));
             BigNumber.StdBigNumber xbigTN = new BigNumber.StdBigNumber((long)(TN ));
             BigNumber.StdBigNumber xbigFN = new BigNumber.StdBigNumber((long)(FN ));

            BigNumber.StdBigNumber xn = (xbigTP * xbigTN) - (xbigFP * xbigFN);
            BigNumber.StdBigNumber xd = (xbigTP + xbigFP) * (xbigTP + xbigFN) * (xbigTN + xbigFP) * (xbigTN + xbigFN);
            BigNumber.StdBigNumber sqsd = (BigNumber.StdBigNumber)BigNumber.BigNumberMath.SquareRoot(xd);

            double xsmallMcc = double.Parse(xn.ToString()) / double.Parse(sqsd.ToString());

        //    double fastMCC = MCCFast;

       //     double d = Math.Abs(fastMCC - xsmallMcc);
       //     if (d > 0.000001)
      //         throw new Exception("too different");

            return xsmallMcc;
         }
      }

      public double _MCC
      {
          get
          {
              // return MCCFast;


              if (Accuracy > 1d - SmallNumber)
              {
                  return 1;
              }
              if (Accuracy < SmallNumber)
              {
                  return -1;
              }

              if ((TP + FP == 0) || (TP + FN == 0) || (TN + FP == 0) || (TN + FN == 0))
                  return 0;

              double xbigTP = (double)TP ;//000d;
              double xbigFP = (double)FP ;//0000d;
              double xbigTN = (double)TN ;//0000d;
              double xbigFN = (double)FN ;// 100000d;
          
             double xn = (xbigTP * xbigTN) - (xbigFP * xbigFN);
             double xd = (xbigTP + xbigFP) * (xbigTP + xbigFN) * (xbigTN + xbigFP) * (xbigTN + xbigFN);
             double sqsd = Math.Sqrt(xd);

             double xsmallMcc = xn / xd;// double.Parse(xn.ToString()) / double.Parse(sqsd.ToString());

              //    double fastMCC = MCCFast;

              //     double d = Math.Abs(fastMCC - xsmallMcc);
              //     if (d > 0.000001)
              //         throw new Exception("too different");

              return xsmallMcc;
          }
      }
      double SmallNumber = 0.000000001;
      public double? StoredMCCFast = null;
      public bool PredictedClass;
      public bool ExpectedClass;
     
      public double MCCFast
      {
         get
         {
            
             if (Accuracy > 1d - SmallNumber)
             {
                 return 1;
             }
             if (Accuracy < SmallNumber)
             {
                 return -1;
             }
             

            if ((TP + FP == 0) || (TP + FN == 0) || (TN + FP == 0) || (TN + FN == 0))
               return 0;

            try
            {

                checked
                {
                    decimal xn = (decimal)((TP*1) * TN  ) - (FP * FN);
                    decimal xd = ((TP * 1) + FP) * ((TP * 1) + FN) * (TN + FP) * (TN + FN);
                    decimal sqsd = Isqrt(xd);

                    double xsmallMcc = (double)xn / (double)sqsd;

                    if (xsmallMcc < -1 || xsmallMcc > 1)
                        throw new Exception("out of range!");
                    StoredMCCFast = xsmallMcc;
                    return xsmallMcc;
                }
            }
            catch (OverflowException e)
            {
                StoredMCCFast = MCC;
                return (double)StoredMCCFast;
            }
         }
      }

      public ConfusionMatrix Clone()
      {
         ConfusionMatrix CM = new ConfusionMatrix();
         CM.FP = this.FP;
         CM.TP = this.TP;
         CM.FN = this.FN;
         CM.TN = this.TN;
         return CM;
      }

      internal void Add(ConfusionMatrix Other)
      {
         this.TP += Other.TP;
         this.FP += Other.FP;
         this.FN += Other.FN;
         this.TN += Other.TN;
      }
   }
}