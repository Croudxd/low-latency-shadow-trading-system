trade:([] time:`long$(); size:`long$(); price:`long$(); order_type:`byte$())

report:([] order_id:`long$(); last_quantity:`long$(); last_price:`long$(); leaves_quantity:`long$(); trade_id:`long$(); timestamp:`long$(); status:`byte$(); side:`byte$(); reject_code:`byte$())

candle:([] sym:`symbol$(); open:`long$(); high:`long$(); low:`long$(); close:`long$(); volume:`long$(); time:`long$())

order:([] id:`long$(); size:`long$(); price:`long$(); side:`byte$(); action:`byte$(); status:`byte$())
