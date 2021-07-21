package Altibase.jdbc.driver.sharding.util;

// PROJ-2733 DistTxInfo for verification
public class DistTxInfoForVerify implements Cloneable
{
    public long mEnvSCN;
    public long mSCN;
    public long mTxFirstStmtSCN;
    public long mTxFirstStmtTime;
    public byte mDistLevel;
    
    @Override
    public Object clone() throws CloneNotSupportedException
    {
        return super.clone();
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("DistTxInfoForVerify { ");
        sSb.append("   mEnvSCN          = ").append(mEnvSCN);
        sSb.append(" , mSCN             = ").append(mSCN);
        sSb.append(" , mTxFirstStmtSCN  = ").append(mTxFirstStmtSCN);
        sSb.append(" , mTxFirstStmtTime = ").append(mTxFirstStmtTime);
        sSb.append(" , mDistLevel       = ").append(mDistLevel);
        sSb.append(" }");

        return sSb.toString();
    }
}

