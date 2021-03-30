# DHEMS Mathematical formula

###### tags: `DHEMS`

## Community Enerage Management System (CEMS)

* $P^{j}_{grid}$ 第 j 個時刻之社區市電功率
* $\rho_{b}^{j}$ 第 j 個時刻之時間電價
* $\rho_{f}^{j}$ 第 j 個時刻之回饋報價 (定值)
* $\epsilon_{it}^{s}$ 即時備轉開啟時刻
* $\epsilon_{it}^{e}$ 即時備轉結束時刻
* $d$ 抑低用電日
* $P_{grid}^{j,d}$ 第d天第j時刻的市電功率
* $P_{max}^{d}$ 第d天抑低用電時段市電最大功率
* $P_{grid}^{avg}$ 抑低用電時段平均值 (基準用電容量 CBL)

$$ P_{max}^{d}= \max_{j=\epsilon_{it}^{s},...,\epsilon_{it}^{e}}P_{grid}^{j,d}, \quad d=1,...,5 $$
$$ P_{grid}^{avg} = \frac{\sum_{d=1}^{D} P_{max}^{d}} {D} $$

* Objective Function
$$
\min_{\substack {
    P_{grid}^{j}, j=0,...,N-1 \\ 
    \mu_{grid}^{j}, j=0,...,N-1 \\
    P_{sell}^{j}, j=0,...,N-1 \\ 
    P_{ESS}^{j}, j=0,...,N-1 \\
    P_{FC}^{j}, j=0,...,N-1 \\ 
    \mu_{FC}^{j}, j=0,...,N-1 \\
    P_{FC\_ON}^{j}, j=0,...,N-1 \\
    P_{FC\_OFF}^{j}, j=0,...,N-1 \\
    z_{i}^{j}, i=0,...,M-1,~j=0,...,N-1 \\
    \lambda_{i}^{j}, i=0,...,M-1,~j=0,...,N-1 \\
}}
\sum_{j=k}^{N} \rho_{b}^{j} (P^{j}_{grid} - P_{sell}^{j})T_{s} -
\sum_{j=\epsilon_{it}^{s}}^{\epsilon_{it}^{e}} \rho_{f}^{j} (P_{grid}^{avg}-P^{j}_{grid}) T_{s}
$$

* Constraint
  
  * Deamnd Response

    $$ E_{s} \leq \sum_{j = \epsilon_{it}^{s}}^{\epsilon_{it}^{e}} (P_{grid}^{avg} - P_{grid}^{j})T_{s} $$
  
  * Blanced Function
    
    $$ P^{j}_{grid} + P^{j}_{PV} + P^{j}_{FC} - P^{j}_{sell} - P^{j}_{ESS} = \sum_{u=1}^{U}(\sum_{a \in  A_{c1} \cup A_{c2} \cup A_{c3}} P_{u,a}^{j} + \sum_{a \in A_{uc}} P_{u, uc}^{j}) $$

  * Grid & Sell
  <!-- * $P_{u, grid}^{j}$ 第 u 個住戶在第 j 個時刻消耗的市電功率 -->
  <!-- * $P_{u, grid}^{max}$ 第 u 個住戶的市電最大功率限制 -->
  <!-- * $\alpha_{u}^{j}$ 第 u 個住戶在第 j 個時刻同意使用多少百分比之市電功率 -->
    $$ P_{grid}^{max} = \sum_{u=1}^{U} \alpha_{u}^{j} P_{u, grid}^{max} $$
    $$ 0 \leq P_{grid}^{j} \leq P_{grid}^{max} $$

    $$ P_{grid}^{j} \leq \mu_{grid}^{j}P_{grid}^{max} $$

    $$ P_{sell}^{j} \leq [1 - \mu_{grid}^{j}] P_{sell}^{max} $$
    
    $$ P_{sell}^{j} \leq P_{FC}^{j} + P_{PV}^{j} $$

    <!-- * For GHEMS => LHEMS
    $$ P_{grid}^{max} = \sum_{u=1}^{U} \alpha_{u}^{j} P_{u, grid}^{max} $$
    $$ P_{grid}^{j} = \sum_{u=1}^{U} \alpha_{u}^{j} P_{u, grid}^{j} $$
    $$ 0 \leq P_{u,grid}^{j} \leq P_{u, grid}^{max} $$
    $$ 0 \leq \alpha_{u}^{j} \leq 1 $$ -->

  * Battery
  
    <!-- * $P_{u,ESS}^{j}$ 第 u 個住戶在第 j 個時刻使用的電池功率 -->
    <!-- * $\beta_{u}^{j}$ 第 u 個住戶在第 j 個時刻同意使用多少百分比之電池功率 -->

    $$P_{discharge}^{max} \leq P^{j}_{ESS} \leq P_{charge}^{max}$$

    $$SOC^{min} \leq SOC_{j-1} + \frac {P^{j}_{ESS}T_{s}}{C_{ESS}V_{ESS}} \leq SOC^{max}$$

    $$SOC_{j-1} + \sum_{j=k}^{T-1} \frac{P^{j}_{ESS}T_{s}}{C_{ESS}V_{ESS}} \geq SOC^{threshold}$$

    $$SOC_{j} = SOC_{j-1} + \frac {P^{j}_{ESS}T_{s}}{C_{ESS}V_{ESS}}$$

    <!-- * For GHEMS => LHEMS
    $$ P_{ESS}^{j} = \sum_{u=1}^{U} \beta_{u}^{j} P_{u,ESS}^{j} $$
    $$ P_{u,discharge}^{max} \leq P_{u,ESS}^{j} \leq P_{u,charge}^{max} $$
    $$ 0 \leq \beta_{u}^{j} \leq 1 $$ -->

  <!-- * FC -->

    <!-- $$ 0 \leq P^{j}_{FC} \leq P_{FC\_max}$$ -->

    <!-- $$ P^{j}_{FC\_ON} + P^{j}_{FC\_OFF} = P^{j}_{FC}$$ -->

    <!-- $$ P^{j}_{FC\_ON} \leq \mu^{j}_{FC}P_{FC\_max}$$ -->

    <!-- $$ \mu^{j}_{FC}P_{FC\_min} \leq P^{j}_{FC\_ON}$$ -->

    <!-- $$ P^{j}_{FC\_OFF} \leq [1 - \mu^{j}_{FC}]P_{FC\_OFF}$$ -->

    <!-- $$ 0 \leq \lambda^{j}_{i} \leq z^{j}_{i} \quad i=0,1,...,m-1$$ -->

    <!-- $$ \sum_{i=0}^{m-1} z^{j}_{i} =1$$ -->

    <!-- $$ \begin{aligned}
    P^{j}_{FC} &= 0 \cdot z^{j}_{0} \\
    &+ 0.35 \cdot z^{j}_{1} + (1.545-0.35) \cdot \lambda^{j}_{1} \\
    &+ 1.545 \cdot z^{j}_{2} + (2.740-1.545) \cdot \lambda^{j}_{2} \\
    &+ 3.935 \cdot z^{j}_{3} + (3.935-2.740) \cdot \lambda^{j}_{3} \\
    &+ 5.130 \cdot z^{j}_{4} + (5.130-3.935) \cdot \lambda^{j}_{4} \\
    \end{aligned} $$ -->

    <!-- $$ \begin{aligned}
    P^{j}_{FCT} &= 0 \cdot z^{j}_{0} \\
    &+ 0.6283 \cdot z^{j}_{1} + (3.0617-0.6283) \cdot \lambda^{j}_{1} \\
    &+ 3.0617 \cdot z^{j}_{2} + (5.8717-3.0617) \cdot \lambda^{j}_{2} \\
    &+ 5.8717 \cdot z^{j}_{3} + (9.0692-8.8717) \cdot \lambda^{j}_{3} \\
    &+ 9.0692 \cdot z^{j}_{4} + (12.8214-9.0692) \cdot \lambda^{j}_{4} \\
    \end{aligned} $$ -->

---

## Home Enerage Management System (HEMS)
* $u$ 代表第u個家庭
* $a$ 代表第a個設備負載
* $j$ 代表第j個時刻
* $P^{j}_{u,grid}$ 第 u 個住戶在第 j 個時刻消耗的市電功率

$$ \min_{\substack{
    r_{u,a}^{j},~j=0,...,N-1,~a \in A_{u, c1} \cup A_{u, c2} \cup A_{u, c3}\\ 
    \delta_{u,a}^{j},~j=0,...,N-1,~a \in A_{u, c2} \cup A_{u, c3}\\ 
    P_{u, a}^{j},~j=0,...,N-1,~a \in A_{u, c3}\\
    P_{u, grid}^{j},~j=0,...,N-1
}}
\sum_{j=k}^{N} \rho_{b}^{j}P^{j}_{u,grid} T_{s} -
\sum_{j=\epsilon_{it}^{s}}^{\epsilon_{it}^{e}} \rho_{f}^{j}P^{j}_{u,grid} T_{s} 
$$

<!-- HEMS Constraint -->
* Constraint
 
  * Demand response
  
    $$ 0 \leq \alpha_{u}^{j} \leq 1 $$

    $$ \omega_{u}^{j} = \sum_{u=1}^{U} P_{u, grid}^{j} $$

    $$ \omega_{total}^{j} = \sum_{u=1}^{U} P_{u, grid}^{j} $$

    $$ 0 \leq \alpha_{u}^{j} \leq \frac{P_{u,grid}^{max}-\frac{\omega_{u}^{j}}{\omega_{total}^{j}}*E_{s}}{P_{u,grid}^{max}}, \qquad \forall j \in [\epsilon_{it}^{s}, \epsilon_{it}^{e}] $$
    
    $$ \alpha_{u}^{j} = 1, \qquad  \forall j \in [0, N-1] \backslash  [\epsilon_{it}^{s}, \epsilon_{it}^{e}] $$

  * Balanced Function
    * $A_{u,uc}$第u個住戶的不可控負載類型
    * $P_{u,uc}^{j}$第u個住戶在第j個時刻的不可控負載功率

    $$ P_{u,grid}^{j} - P_{u,ESS}^{j}= \sum_{a \in A_{u, c1} \cup A_{u, c2} \cup A_{u, c3}} P_{u,a}^{j}  + \sum_{a \in A_{u, uc}} P_{u,uc}^{j}$$

  * Pgrid

  $$ 0 \leq P_{u,grid}^{j} \leq \alpha_{u}^{j}P_{u, grid}^{max} $$

  * Battery
  
  $$ P_{u,charge}^{max} = \frac{P_{charge}^{max}}{U}$$
  
  $$ P_{u,discharge}^{max} = \frac{P_{discharge}^{max}}{U}$$
  
  $$ V_{u,ESS} = \frac{V_{ESS}}{U}$$
  
  $$ P_{u,discharge}^{max} \leq P_{u,ESS}^{j} \leq P_{u,charge}^{max} $$

  $$ SOC^{min} \leq SOC_{j-1} + \frac {P^{j}_{u,ESS}T_{s}}{C_{ESS}V_{u,ESS}} \leq SOC^{max} $$

  $$ SOC_{j-1} + \sum_{j=k}^{T-1} \frac{P^{j}_{u,ESS}T_{s}}{C_{ESS}V_{u,ESS}} \geq SOC^{threshold} $$

  $$ SOC_{j} = SOC_{j-1} + \frac {P^{j}_{u,ESS}T_{s}}{C_{ESS}V_{u,ESS}} $$

  * Loads Constraint
      <!-- Uncontrollable load Constraint -->
      * Uncontrollable load
      
      $$ P_{u,uc}^{j} =
      \left\{ 
        \begin{array}
          rr_{u,uc}^{j} P_{u,uc}^{}, \qquad &\forall k \in [\tau_{u}^{s}, \tau_{u}^{e}]\\
          0 \qquad, \qquad &otherwise
        \end{array}
      \right.
      $$

      $$ r_{u,uc}^{j} \in \{0,1\}, \qquad \forall j \in [\tau_{u}^{s}, \tau_{u}^{e}] $$
      $$ P_{u,uc}^{} \in [0.3, 1.0] $$
      
      <!-- Deploeyment load Constraint -->
      * Deploeyment load

      $$ P_{u,a}^{j} =
      \left\{ 
        \begin{array}
          rr_{u,a}^{j} P_{u,a}^{max}, \qquad &\forall k \in [\tau_{u,a}^{s}, \tau_{u,a}^{e}]\\
          0 \qquad, \qquad &otherwise
        \end{array}
      \right.
      $$

      $$ r_{u,a}^{j} \in \{0,1\}, \qquad \forall j \in [\tau_{u,a}^{s}, \tau_{u,a}^{e}] $$

      $$ r_{u,a}^{j} = 0, \qquad \forall j \in [0,N-1] \backslash [\tau_{u,a}^{s}, \tau_{u,a}^{e}] $$

      $$ \forall a \in A_{u,c1} $$

      $$ \sum_{k=0}^{T-1} r_{u,a}^{j} \geq Q_{a} $$

      $$ \forall a \in A_{u,c2} \cup A_{u,c3} $$

      $$ \sum_{k= \tau_{u,a}^{s}}^{\tau_{u,a}^{e}- \Gamma_{u, a} - 1} \delta_{u,a}^{j} = 1 $$

      $$ r_{u,a}^{j+n} \geq \delta_{u,a}^{j}, \qquad n = 0,...,\Gamma_a-1 $$

      $$ \forall a \in A_{u,c3} $$

      $$ \psi_{u, a}^{j+n} \geq \delta_{u,a}^{j} \sigma_{u,a}^{n} $$
