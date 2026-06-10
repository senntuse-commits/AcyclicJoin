-- start query 1 in stream 0 using template query16.tpl
SELECT  
   COUNT(DISTINCT cs_order_number) AS "order count",
   SUM(cs_ext_ship_cost) AS "total shipping cost",
   SUM(cs_net_profit) AS "total net profit"
FROM
   catalog_sales cs1
JOIN date_dim ON cs1.cs_ship_date_sk = d_date_sk
JOIN customer_address ON cs1.cs_ship_addr_sk = ca_address_sk
JOIN call_center ON cs1.cs_call_center_sk = cc_call_center_sk
WHERE
    d_date BETWEEN '1999-02-01' AND ('1999-02-01'::date + INTERVAL '60 days')
  AND ca_state = 'IL'
  AND cc_county IN ('Williamson County', 'Williamson County', 'Williamson County', 'Williamson County', 'Williamson County')
  AND EXISTS (
      SELECT 1
      FROM catalog_sales cs2
      WHERE cs1.cs_order_number = cs2.cs_order_number
        AND cs1.cs_warehouse_sk <> cs2.cs_warehouse_sk
   )
  AND NOT EXISTS (
      SELECT 1
      FROM catalog_returns cr1
      WHERE cs1.cs_order_number = cr1.cr_order_number
   )
ORDER BY COUNT(DISTINCT cs_order_number)
LIMIT 100;
-- end query 1 in stream 0 using template query16.tpl
