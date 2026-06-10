-- start query 1 in stream 0 using template query92.tpl
WITH avg_discount AS (
    SELECT 
        ws_item_sk, 
        1.3 * AVG(ws_ext_discount_amt) AS avg_discount_amt
    FROM 
        web_sales 
        JOIN date_dim ON d_date_sk = ws_sold_date_sk
    WHERE 
        d_date BETWEEN '1998-03-18' AND (CAST('1998-03-18' AS date) + INTERVAL '90 days')
    GROUP BY 
        ws_item_sk
)
SELECT  
    SUM(ws_ext_discount_amt) AS "Excess Discount Amount"
FROM 
    web_sales 
    JOIN item ON i_item_sk = ws_item_sk
    JOIN date_dim ON d_date_sk = ws_sold_date_sk
    JOIN avg_discount ON web_sales.ws_item_sk = avg_discount.ws_item_sk
WHERE
    i_manufact_id = 269
    AND d_date BETWEEN '1998-03-18' AND (CAST('1998-03-18' AS date) + INTERVAL '90 days')
    AND ws_ext_discount_amt > avg_discount.avg_discount_amt
GROUP BY
    i_manufact_id
ORDER BY 
    SUM(ws_ext_discount_amt) DESC
LIMIT 100;

-- end query 1 in stream 0 using template query92.tpl
