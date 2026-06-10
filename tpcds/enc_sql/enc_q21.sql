-- start query 1 in stream 0 using template query21.tpl
SELECT *
FROM (
    SELECT w_warehouse_name,
           i_item_id,
           SUM(CASE WHEN d_date < enc_timestamp_encrypt('1998-04-08') THEN inv_quantity_on_hand ELSE '0' END) AS inv_before,
           SUM(CASE WHEN d_date >= enc_timestamp_encrypt('1998-04-08') THEN inv_quantity_on_hand ELSE '0' END) AS inv_after
    FROM inventory
    JOIN warehouse ON inv_warehouse_sk = w_warehouse_sk
    JOIN item ON i_item_sk = inv_item_sk
    JOIN date_dim ON inv_date_sk = d_date_sk
    WHERE i_current_price BETWEEN enc_float4_encrypt('0.99') AND enc_float4_encrypt('1.49')
      AND d_date BETWEEN enc_timestamp_encrypt('1998-03-09') AND enc_timestamp_encrypt('1998-05-08')
    GROUP BY w_warehouse_name, i_item_id
) x
WHERE (CASE WHEN inv_before > enc_int4_encrypt('0') THEN inv_after / inv_before ELSE NULL END) BETWEEN enc_int4_encrypt('2')/enc_int4_encrypt('3') AND enc_int4_encrypt('3')/enc_int4_encrypt('2')
ORDER BY w_warehouse_name, i_item_id
LIMIT 100;
-- end query 1 in stream 0 using template query21.tpl
