import streamlit as st
import pykx as kx
import pandas as pd

st.set_page_config(page_title="Shadow Trading UI", layout="wide")
st.title("Live Shadow Trading System")

@st.cache_data(ttl=1)
def get_kdb_data():
    try:
        with kx.SyncQConnection(host='localhost', port=5001) as conn:
            
            q_summary = "select VWAP: size wavg price, TotalVol: sum size by sym from trade"
            summary_df = conn(q_summary).pd()
            
            q_raw = "-100#select from candle"
            raw_df = conn(q_raw).pd()
            
            return summary_df, raw_df
            
    except Exception as e:
        st.error(f"kdb+ Connection Failed: {e}")
        return pd.DataFrame(), pd.DataFrame()

summary_data, raw_data = get_kdb_data()

if not summary_data.empty:
    st.subheader("Live Aggregations (Calculated in kdb+)")
    st.dataframe(summary_data, use_container_width=True)

# st.subheader("Recent Price Action")
#     if 'time' in raw_data.columns and 'price' in raw_data.columns:
#         st.scatter_chart(raw_data, x='time', y='price', color='sym')

    st.subheader("Raw Ticker Tape (Last 100 Trades)")
    st.dataframe(raw_data, use_container_width=True)
    
    if st.button("Refresh Data"):
        st.rerun()
else:
    st.warning("No data found! Is kdb+ running on port 5001 and is the C++ feeder sending data?")
