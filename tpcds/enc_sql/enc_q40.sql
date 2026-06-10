-- start query 1 in stream 0 using template query40.tpl
SELECT
    w_state,
    i_item_id,
    SUM(CASE WHEN (d_date < enc_timestamp_encrypt('1998-04-08')) 
              THEN cs_sales_price - cr_refunded_cash ELSE '0' END) AS sales_before,
    SUM(CASE WHEN (d_date >= enc_timestamp_encrypt('1998-04-08')) 
              THEN cs_sales_price -cr_refunded_cash ELSE '0' END) AS sales_after
FROM
    catalog_sales
    LEFT OUTER JOIN catalog_returns ON (cs_order_number = cr_order_number AND cs_item_sk = cr_item_sk),
    warehouse,
    item,
    date_dim
WHERE
    i_current_price BETWEEN '0.99' AND '1.49'
    AND i_item_sk = cs_item_sk
    AND cs_warehouse_sk = w_warehouse_sk
    AND cs_sold_date_sk = d_date_sk
    AND d_date BETWEEN enc_timestamp_encrypt('1998-05-08')
                   AND enc_timestamp_encrypt('1998-05-08')
GROUP BY
    w_state, i_item_id
ORDER BY
    w_state, i_item_id
LIMIT 100;
-- end query 1 in stream 0 using template query40.tpl
