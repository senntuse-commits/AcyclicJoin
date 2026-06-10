-- start query 1 in stream 0 using template query94.tpl
select  
    count(distinct ws1.ws_order_number) as "order count",
    sum(ws1.ws_ext_ship_cost) as "total shipping cost",
    sum(ws1.ws_net_profit) as "total net profit"
from
    web_sales ws1
    join date_dim on ws1.ws_ship_date_sk = d_date_sk
    join customer_address on ws1.ws_ship_addr_sk = ca_address_sk
    join web_site on ws1.ws_web_site_sk = web_site_sk
where
    d_date between '1999-05-01' and (cast('1999-05-01' as date) + interval '60 days')
    and ca_state = 'TX'
    and web_company_name = 'pri'
    and exists (
        select 1
        from web_sales ws2
        where ws1.ws_order_number = ws2.ws_order_number
        and ws1.ws_warehouse_sk <> ws2.ws_warehouse_sk
    )
    and not exists (
        select 1
        from web_returns wr1
        where ws1.ws_order_number = wr1.wr_order_number
    )
group by 
    ca_state
order by 
    "order count" desc
limit 100;

-- end query 1 in stream 0 using template query94.tpl
