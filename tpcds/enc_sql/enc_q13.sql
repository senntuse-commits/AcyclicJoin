-- start query 1 in stream 0 using template query13.tpl
select avg(ss_quantity)
       ,avg(ss_ext_sales_price)
       ,avg(ss_ext_wholesale_cost)
       ,sum(ss_ext_wholesale_cost)
 from store_sales
     ,store
     ,customer_demographics
     ,household_demographics
     ,customer_address
     ,date_dim
 where s_store_sk = ss_store_sk
 and  ss_sold_date_sk = d_date_sk and d_year = enc_int4_encrypt('2001')
 and((ss_hdemo_sk=hd_demo_sk
  and cd_demo_sk = ss_cdemo_sk
  and cd_marital_status = 'D'
  and cd_education_status = '2 yr Degree'
  and ss_sales_price between enc_float4_encrypt('100.00') and enc_float4_encrypt('150.00')
  and hd_dep_count = enc_int4_encrypt('3')   
     )or
     (ss_hdemo_sk=hd_demo_sk
  and cd_demo_sk = ss_cdemo_sk
  and cd_marital_status = 'S'
  and cd_education_status = 'Secondary'
  and ss_sales_price between enc_float4_encrypt('50.00') and enc_float4_encrypt('100.00')   
  and hd_dep_count = enc_int4_encrypt('1')
     ) or 
     (ss_hdemo_sk=hd_demo_sk
  and cd_demo_sk = ss_cdemo_sk
  and cd_marital_status = 'W'
  and cd_education_status = 'Advanced Degree'
  and ss_sales_price between enc_float4_encrypt('150.00') and enc_float4_encrypt('200.00') 
  and hd_dep_count = enc_int4_encrypt('1')  
     ))
 and((ss_addr_sk = ca_address_sk
  and ca_country = 'United States'
  and ca_state in ('CO', 'IL', 'MN')
  and ss_net_profit between enc_float4_encrypt('100') and enc_float4_encrypt('200')  
     ) or
     (ss_addr_sk = ca_address_sk
  and ca_country = 'United States'
  and ca_state in ('OH', 'MT', 'NM')
  and ss_net_profit between enc_float4_encrypt('150') and enc_float4_encrypt('300')  
     ) or
     (ss_addr_sk = ca_address_sk
  and ca_country = 'United States'
  and ca_state in ('TX', 'MO', 'MI')
  and ss_net_profit between enc_float4_encrypt('50') and enc_float4_encrypt('250')  
     ))
;

-- end query 1 in stream 0 using template query13.tpl
