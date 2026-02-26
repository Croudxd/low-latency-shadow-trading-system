import streamlit as st
import pykx as kx
import pandas as pd
import plotly.graph_objects as go
import time

st.set_page_config(page_title="Shadow Trading UI", layout="wide")
st.title("Live Candlestick Chart")

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
    st.subheader("BTC Price Action")
    
    price_cols = ['open', 'high', 'low', 'close']
    candle_data[price_cols] = candle_data[price_cols] / 100.0
    
    if 'volume' in candle_data.columns:
        candle_data['volume'] = candle_data['volume'] / 1000000.0
    
    if 'time' in candle_data.columns:
        candle_data['time'] = pd.to_datetime(candle_data['time'], unit='ns')
        x_axis = candle_data['time'].dt.strftime('%H:%M:%S.%f')
    else:
        x_axis = candle_data.index.astype(str)
        
    fig = go.Figure(data=[go.Candlestick(
        x=x_axis,
        open=candle_data['open'],
        high=candle_data['high'],
        low=candle_data['low'],
        close=candle_data['close'],
        increasing_line_color='#008000',
        decreasing_line_color='#ff0000'
    )])
    
    fig.update_layout(
        xaxis_rangeslider_visible=False,
        margin=dict(l=0, r=0, t=30, b=0),
        height=500
    )
    
    fig.update_xaxes(type='category', nticks=10)
    
    st.plotly_chart(fig, width="stretch")
    
else:
    st.warning("Waiting for candle data...")

time.sleep(1)
st.rerun()
