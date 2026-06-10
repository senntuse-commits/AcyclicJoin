-- start query 1 in stream 0 using template query18.tpl
select  i_item_id,
        ca_country,
        ca_state, 
        ca_county,
        avg(cs_quantity) agg1,
        avg(cs_list_price) agg2,
        avg(cs_coupon_amt) agg3,
        avg(cs_sales_price) agg4,
        avg(cs_net_profit) agg5,
        avg(c_birth_year) agg6,
        avg(cd1.cd_dep_count) agg7
 from catalog_sales, customer_demographics cd1, 
      customer_demographics cd2, customer, customer_address, date_dim, item
 where cs_sold_date_sk = d_date_sk and
       cs_item_sk = i_item_sk and
       cs_bill_cdemo_sk = cd1.cd_demo_sk and
       cs_bill_customer_sk = c_customer_sk and
       cd1.cd_gender = 'M' and 
       cd1.cd_education_status = 'College' and
       c_current_cdemo_sk = cd2.cd_demo_sk and
       c_current_addr_sk = ca_address_sk and
       c_birth_month in (enc_int4_encrypt('9'),enc_int4_encrypt('5'),enc_int4_encrypt('12'),enc_int4_encrypt('4'),enc_int4_encrypt('1'),enc_int4_encrypt('10')) and
       d_year = enc_int4_encrypt('2001') and
       ca_state in ('ND','WI','AL'
                   ,'NC','OK','MS','TN')
 group by rollup (i_item_id, ca_country, ca_state, ca_county)
 order by ca_country,
        ca_state, 
        ca_county,
	i_item_id
 limit 100;

-- end query 1 in stream 0 using template query18.tpl
