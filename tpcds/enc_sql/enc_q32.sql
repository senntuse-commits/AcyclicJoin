-- start query 1 in stream 0 using template query32.tpl
select  sum(cs_ext_discount_amt)  as "excess discount amount" 
from 
   catalog_sales 
   ,item 
   ,date_dim
where
i_manufact_id = enc_int4_encrypt('269')
and i_item_sk = cs_item_sk 
and d_date between enc_timestamp_encrypt('1998-03-18') and 
         enc_timestamp_encrypt('1998-06-16')
and d_date_sk = cs_sold_date_sk 
and cs_ext_discount_amt  
     > ( 
         select 
            enc_float4_encrypt('1.3')
            -- enc_float4_encrypt('1.3') * avg(cs_ext_discount_amt) 
         from 
            catalog_sales 
           ,date_dim
         where 
              cs_item_sk = i_item_sk 
          and d_date between enc_timestamp_encrypt('1998-03-18') and 
                    enc_timestamp_encrypt('1998-06-16')
          and d_date_sk = cs_sold_date_sk 
      ) 
limit 100;

-- end query 1 in stream 0 using template query32.tpl
