import streamlit as st
import pykx as kx
import pandas as pd

st.set_page_config(page_title="Shadow Trading UI", layout="wide")
st.title("Live Candle Graph")

@st.cache_data(ttl=1)
def get_candle_data():
    try:
        with kx.SyncQConnection(host='localhost', port=5001) as conn:
            return conn("select from candle").pd()
    except Exception as e:
        st.error(f"kdb+ Connection Failed: {e}")
        return pd.DataFrame()

candle_data = get_candle_data()

if not candle_data.empty:
    st.subheader("ETH Price Action (Row by Row)")
    
    chart_display = candle_data[['open', 'high', 'low', 'close']]
    
    st.line_chart(chart_display, width="stretch")
    
    st.subheader("Raw Data")
    st.dataframe(candle_data, width="stretch")
    
    if st.button("Refresh"):
        st.rerun()
else:
    st.warning("Waiting for candle data...")
