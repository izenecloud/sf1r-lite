
top_dir = File.dirname(File.expand_path(__FILE__))
env_sh = File.join(top_dir, "b5m_env.sh")
IO.readlines(env_sh).each do |line|
  env_a = line.split("=")
  next unless env_a.size==2
  env_b = env_a[0].split(" ")
  next unless env_b.size>=2
  name = env_b[1].strip
  value = env_a[1].strip
  ENV[name]=value
end

